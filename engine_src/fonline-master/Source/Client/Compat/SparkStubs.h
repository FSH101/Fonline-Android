// Minimal SPARK compatibility stubs for Android builds without the SPARK SDK.
#pragma once

#include "Common.h"
#include "Rendering.h"

FO_BEGIN_NAMESPACE();
class ParticleManager;
FO_END_NAMESPACE();

namespace SPK
{
    class System;

    template<typename T>
    using Ref = std::shared_ptr<T>;

    namespace FO
    {
        class SparkRenderBuffer final : public RenderBuffer
        {
        public:
            explicit SparkRenderBuffer(size_t /*vertices*/)
            {
            }

            void PositionAtStart()
            {
            }

            void SetNextVertex(const Vector3D& /*pos*/, const Color& /*color*/)
            {
            }

            void SetNextTexCoord(float32 /*tu*/, float32 /*tv*/)
            {
            }

            void Render(size_t /*vertices*/, RenderEffect* /*effect*/) const
            {
            }
        };

        class SparkQuadRenderer final : public Renderer
        {
        public:
            static auto Create() -> Ref<SparkQuadRenderer>
            {
                return {};
            }

            ~SparkQuadRenderer() override = default;

            void Setup(string_view /*path*/, ParticleManager& /*particle_mngr*/)
            {
            }

            auto GetDrawWidth() const -> int32
            {
                return 0;
            }

            auto GetDrawHeight() const -> int32
            {
                return 0;
            }

            void SetDrawSize(int32 /*width*/, int32 /*height*/)
            {
            }

            auto GetEffectName() const -> const string&
            {
                static const string empty_effect {};
                return empty_effect;
            }

            void SetEffectName(const string& /*effect_name*/)
            {
            }

            auto GetTextureName() const -> const string&
            {
                static const string empty_texture {};
                return empty_texture;
            }

            void SetTextureName(const string& /*tex_name*/)
            {
            }

        private:
            SparkQuadRenderer() :
                Renderer(false)
            {
            }

            explicit SparkQuadRenderer(bool needs_dataset) :
                Renderer(needs_dataset)
            {
            }

            SparkQuadRenderer(const SparkQuadRenderer& renderer) = default;
        };
    }
}
