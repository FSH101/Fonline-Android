package com.example.fonline

import android.content.Context
import android.util.AttributeSet
import android.view.MotionEvent
import android.view.SurfaceHolder
import android.view.SurfaceView

class GameSurfaceView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null
) : SurfaceView(context, attrs), SurfaceHolder.Callback {

    init {
        holder.addCallback(this)
        isFocusable = true
        isFocusableInTouchMode = true
    }

    fun onHostResume() {
        NativeBridge.nativeOnResume()
    }

    fun onHostPause() {
        NativeBridge.nativeOnPause()
    }

    fun onHostDestroy() {
        NativeBridge.nativeShutdown()
    }

    override fun surfaceCreated(holder: SurfaceHolder) {
        NativeBridge.ensureInit(context)
        NativeBridge.nativeOnSurfaceCreated(holder.surface)
    }

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
        NativeBridge.nativeOnSurfaceChanged(width, height)
    }

    override fun surfaceDestroyed(holder: SurfaceHolder) {
        NativeBridge.nativeOnSurfaceDestroyed()
    }

    override fun onTouchEvent(event: MotionEvent): Boolean {
        val actionMasked = event.actionMasked
        val actionIndex = event.actionIndex
        val pointerId = event.getPointerId(actionIndex)
        val x = event.getX(actionIndex)
        val y = event.getY(actionIndex)

        val isPrimaryAction = when (actionMasked) {
            MotionEvent.ACTION_DOWN,
            MotionEvent.ACTION_UP,
            MotionEvent.ACTION_CANCEL -> true
            else -> actionIndex == 0
        }

        NativeBridge.nativeOnTouch(actionMasked, pointerId, x, y, isPrimaryAction)
        return true
    }
}
