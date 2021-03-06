//-----------------------------------------------------------------------------
// Copyright (c) 2015 Andrew Mac
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

#include "SkyboxDLL.h"
#include <plugins/plugins_shared.h>

#include <sim/simObject.h>
#include <3d/rendering/common.h>
#include <graphics/utilities.h>

#include <bx/fpumath.h>

using namespace Plugins;

bool skyboxEnabled = false;
bgfx::TextureHandle skyboxTexture = BGFX_INVALID_HANDLE;
bgfx::ProgramHandle skyboxShader = BGFX_INVALID_HANDLE;
bgfx::UniformHandle skyboxMatrixUniform = BGFX_INVALID_HANDLE;

// Called when the plugin is loaded.
void create()
{
   // Load Shader
   Graphics::ShaderAsset* skyboxShaderAsset = Link.Graphics.getShaderAsset("Skybox:skyboxShader");
   if ( skyboxShaderAsset )
   {
      skyboxShader = skyboxShaderAsset->getProgram();
      skyboxMatrixUniform = Link.Graphics.getUniform4x4Matrix("u_mtx", 1);
   }

   // Register Console Functions
   Link.Con.addCommand("Skybox", "load", loadTexture, "", 2, 2);
   Link.Con.addCommand("Skybox", "enable", enableSkybox, "", 1, 1);
   Link.Con.addCommand("Skybox", "disable", disableSkybox, "", 1, 1);
}

void destroy()
{
   
}

void loadTexture(SimObject *obj, S32 argc, const char *argv[])
{
   // Load skybox texture.
   TextureObject* texture_obj = Link.Graphics.loadTexture(argv[1], TextureHandle::BitmapKeepTexture, false, false, false);
   if ( texture_obj )
      skyboxTexture = texture_obj->getBGFXTexture();
}

// Console Functions
void enableSkybox(SimObject *obj, S32 argc, const char *argv[])
{
   skyboxEnabled = true;
}

void disableSkybox(SimObject *obj, S32 argc, const char *argv[])
{
   skyboxEnabled = false;
}

// Per-Frame render function
void render()
{
   if ( !skyboxEnabled || !bgfx::isValid(skyboxTexture) || !bgfx::isValid(skyboxShader) ) 
      return;

   // Deferred + Forward outputs to RenderLayer2 so RenderLayer 1 is under it which
   // is where we want our skybox to render.
   Link.bgfx.setViewClear(Graphics::RenderLayer1,
      BGFX_CLEAR_COLOR,
      0x0000ffff, // Blue for debugging.
      1.0f, 
      0);

   F32 proj[16];
   bx::mtxOrtho(proj, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 100.0f);
   Link.bgfx.setViewTransform(Graphics::RenderLayer1, NULL, proj, BGFX_VIEW_STEREO, NULL);
   Link.bgfx.setViewRect(Graphics::RenderLayer1, 0, 0, *Link.Rendering.canvasWidth, *Link.Rendering.canvasHeight);

   // Calculate view matrix based on current view matrix.
   float viewMtx[16];
   bx::mtxInverse(viewMtx, Link.Rendering.viewMatrix);
   Link.bgfx.setUniform(skyboxMatrixUniform, viewMtx, 1);

   Link.bgfx.setTexture(0, Link.Graphics.getTextureUniform(0), skyboxTexture, UINT32_MAX);
   Link.bgfx.setProgram(skyboxShader);
   Link.bgfx.setState(BGFX_STATE_RGB_WRITE|BGFX_STATE_ALPHA_WRITE, 0);

   // Render skybox as fullscreen quad.
   Link.Graphics.fullScreenQuad(*Link.Rendering.canvasWidth, *Link.Rendering.canvasHeight);

   Link.bgfx.submit(Graphics::RenderLayer1, 0);
}
