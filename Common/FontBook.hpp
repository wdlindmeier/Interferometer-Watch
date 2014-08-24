//
//  FontBook.hpp
//  LIGOclock
//
//  Created by William Lindmeier on 8/14/14.
//
//

#ifndef LIGOclock_FontBook_hpp
#define LIGOclock_FontBook_hpp

#include "cinder/gl/TextureFont.h"

namespace wdl
{

    class FontBook
    {
        FontBook()
        {}
        
        // Don't implement so the singleton can't be copied
        FontBook( FontBook const & );
        void operator = ( FontBook const & );

    protected:
        
        std::map<std::string, ci::gl::TextureFontRef> mSharedFontBook;
        
        static FontBook & getInstance()
        {
            // Instantiated on first use & guaranteed to be destroyed.
            static FontBook instance;
            return instance;
        }
        
    public:
        
        static void loadFont(const std::string & fontName, const std::string & resourceName )
        {
            ci::gl::TextureFontRef daFont = ci::gl::TextureFont::create(ci::Font(ci::app::loadResource(resourceName), 64));
            FontBook::getInstance().mSharedFontBook[fontName] = daFont;
            assert(FontBook::getFont(fontName));
        }
        
        static ci::gl::TextureFontRef getFont( const std::string & fontName )
        {
            return FontBook::getInstance().mSharedFontBook[fontName];
        }
    };
}

#endif
