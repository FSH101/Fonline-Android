package com.example.fonline

import android.os.Bundle
import android.view.WindowManager
import androidx.appcompat.app.AppCompatActivity

class MainActivity : AppCompatActivity() {

    private lateinit var surface: GameSurfaceView

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)

        surface = GameSurfaceView(this)
        setContentView(surface)
    }

    override fun onResume() {
        super.onResume()
        surface.onHostResume()
    }

    override fun onPause() {
        surface.onHostPause()
        super.onPause()
    }

    override fun onDestroy() {
        surface.onHostDestroy()
        super.onDestroy()
    }
}
