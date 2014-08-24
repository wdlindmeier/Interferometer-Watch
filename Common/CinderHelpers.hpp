//
//  TextHelpers.hpp
//  gpuPS
//
//  Created by William Lindmeier on 7/11/14.
//
//

#ifndef gpuPS_TextHelpers_hpp
#define gpuPS_TextHelpers_hpp

#include "cinder/Rect.h"
#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Fbo.h"
#include "cinder/Camera.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include "cinder/gl/TextureFont.h"
#include <boost/algorithm/string.hpp>

namespace cinder
{
    template <typename T>
    static RectT<T> RectMult(RectT<T> rect, float multiplier)
    {
        return RectT<T>(rect.x1 * multiplier, rect.y1 * multiplier, rect.x2 * multiplier, rect.y2 * multiplier);
    }
    
    static Area AreaMult(Area area, float multiplier)
    {
        return Area(area.x1 * multiplier,
                    area.y1 * multiplier,
                    area.x2 * multiplier,
                    area.y2 * multiplier);
    }
}

namespace ligo
{
    static std::string replaceOccurrencesOfString(const std::string & origString,
                                           const std::string & replaceString,
                                           const std::string & withString = "" )
    {
        std::string str(origString);
        size_t index = 0;
        while (true)
        {
            index = str.find(replaceString, index);
            if (index == std::string::npos) break;

            str.replace(index, replaceString.size(), withString);
            
            index += withString.size();
        }
        return str;
    }
    
    static std::string loadFileAsString( const cinder::fs::path & filePath )
    {
        std::ifstream input( filePath.string() );
        // http://stackoverflow.com/questions/116038/what-is-the-best-way-to-slurp-a-file-into-a-stdstring-in-c
        return static_cast<std::stringstream const&>( std::stringstream() << input.rdbuf() ).str();
    }
    
    // This gives us a way to add import statements to shaders so we can use shared code.
    static std::string loadShader( const std::string & shaderName )
    {
        std::string shaderString = loadFileAsString( cinder::app::AppBasic::getResourcePath(shaderName) );
        
        while ( true )
        {
            int importIndex = shaderString.find("#import");
            if ( importIndex == std::string::npos ) break;
            int endlineIndex = shaderString.find("\n", importIndex);
            std::string importLine = shaderString.substr(importIndex, endlineIndex - importIndex);
            std::string filename = ligo::replaceOccurrencesOfString(importLine, "#import");
            filename = ligo::replaceOccurrencesOfString(filename, "\"");
            filename = ligo::replaceOccurrencesOfString(filename, " ");

            std::string importedShader = loadFileAsString( cinder::app::AppBasic::getResourcePath(filename) );
            
            shaderString = ligo::replaceOccurrencesOfString(shaderString, importLine, importedShader);
        }
        return shaderString;
    }
    
    static ci::gl::TextureFontRef DefaultFont;
    
    static ci::gl::TextureFontRef & getDefaultFont()
    {
        if ( !DefaultFont )
        {
            ci::gl::TextureFont::Format format;
            format.enableMipmapping(false);
            DefaultFont = ci::gl::TextureFont::create(ci::Font("Monaco", 64), format);
        }
        return DefaultFont;
    }
    
    const static float kDefaultStringScale = 0.5f;
    
    static ci::Vec2f measureString( const std::string & str,
                                    const float fontScale = kDefaultStringScale,
                                    ci::gl::TextureFontRef font = ci::gl::TextureFontRef() )
    {
        if ( !font )
        {
            font = getDefaultFont();
        }
        ci::Vec2f measurement = font->measureString(str);
        return measurement * fontScale;
    }

    template<typename T>
    static ci::Vec2f drawString( const std::string & str,
                                 const T & pos,
                                 const float fontScale = kDefaultStringScale,
                                 ci::gl::TextureFontRef font = ci::gl::TextureFontRef() )
    {
        if ( !font )
        {
            font = getDefaultFont();
        }
        ci::gl::pushMatrices();
        // Scale to get higher res font
        ci::gl::scale(ci::Vec3f(fontScale, fontScale, fontScale));
        T scaledPosition = pos * (1.0f / fontScale);
        ci::gl::enableAlphaBlending();
        ci::gl::translate(scaledPosition);
        font->drawString(str, ci::Vec2f(0,0));
        ci::gl::popMatrices();
        return ligo::measureString(str, fontScale, font);
    }
    
    template<typename T>
    static ci::Vec2f drawCenteredString( const std::string & str,
                                         const T & pos,
                                         const float fontScale = kDefaultStringScale,
                                         ci::gl::TextureFontRef font = ci::gl::TextureFontRef() )
    {
        ci::Vec2f stringSize = ligo::measureString(str, fontScale, font);
        ci::Vec2f newPos = pos + ci::Vec2f(stringSize.x * -0.5, stringSize.y * 0.5);
        return ligo::drawString(str, newPos, fontScale, font);
    }
    
    static void drawBillboardedString( const ci::CameraPersp cam,
                                   const std::string & str,
                                   const float scale,
                                   const ci::Vec3f & worldPos,
                                   const ci::Vec3f & offset,
                                   ci::gl::TextureFontRef font = ci::gl::TextureFontRef() )
    {
        if ( !font )
        {
            font = getDefaultFont();
        }

        ci::gl::setMatrices( cam );
        
        // Oooohhh, kay. For some reason the inverted model mat doesn't
        // account for the X rotation...
        ci::Matrix44<float> billboardMat = cam.getModelViewMatrix().inverted();
        billboardMat.m[1] = -billboardMat.m[1];
        //    billboardMat.m[5] = -billboardMatHoriz.m[5];
        billboardMat.m[9] = -billboardMat.m[9];
        
        ci::gl::pushMatrices();
        ci::gl::scale(ci::Vec3f(scale, scale, scale));
        ci::Vec3f scaledPosition = worldPos * (1.0f / scale);
        ci::gl::enableAlphaBlending();
        ci::gl::translate(scaledPosition);
        ci::gl::pushMatrices();

        ci::gl::pushModelView();
        ci::gl::multModelView(billboardMat);
        ci::gl::translate(offset);
        ci::gl::scale(ci::Vec3f(1,-1,1));
        
        font->drawString(str, ci::Vec2f(0,0));
        
        ci::gl::popModelView();
        
        ci::gl::popMatrices();
        ci::gl::popMatrices();
    }
    
    // Closure to bind and unbind an FBO
    template <typename Func>
    static inline void DrawToFBO(ci::gl::Fbo & fbo, Func func)
    {
        ci::Area restoreViewport = ci::gl::getViewport();
        fbo.bindFramebuffer();
        ci::Vec2f fboSize = fbo.getBounds().getSize();
        ci::gl::setViewport( fbo.getBounds() );
        ci::gl::setMatricesWindow( fboSize );
        
        func( fboSize );
        
        fbo.unbindFramebuffer();
        // Restore viewport and matrix
        ci::gl::setViewport( restoreViewport );
        ci::gl::setMatricesWindow( ci::app::getWindowSize() );
    }
}

#endif
