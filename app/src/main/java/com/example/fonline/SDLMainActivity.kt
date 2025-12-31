package com.example.fonline

import org.libsdl.app.SDLActivity

class SDLMainActivity : SDLActivity() {
    override fun getLibraries(): Array<String> {
        return arrayOf(
            "SDL3",
            "main"
        )
    }
}
