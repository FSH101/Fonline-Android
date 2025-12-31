package com.example.fonline

import android.content.Context
import android.content.res.AssetManager
import android.view.Surface

object NativeBridge {
    init {
        System.loadLibrary("main")
    }

    @Volatile private var inited = false

    fun ensureInit(context: Context) {
        if (inited) return
        synchronized(this) {
            if (inited) return
            nativeInit(context.assets, context.filesDir.absolutePath)
            inited = true
        }
    }

    external fun nativeInit(assets: AssetManager, filesDirPath: String)
    external fun nativeOnSurfaceCreated(surface: Surface)
    external fun nativeOnSurfaceChanged(width: Int, height: Int)
    external fun nativeOnSurfaceDestroyed()

    external fun nativeOnResume()
    external fun nativeOnPause()
    external fun nativeShutdown()

    external fun nativeOnTouch(
        actionMasked: Int,
        pointerId: Int,
        x: Float,
        y: Float,
        isPrimaryAction: Boolean
    )
}
