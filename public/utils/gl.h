// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: GL Library
//

#pragma once
#include "glad/glad.h"
#include "glres.h"

namespace gl {
    enum class Format {
        RGB, RGBA,
    };

    enum class Type {
        UByte,
    };

    namespace detail {
        template<typename T>
        void Bind(GLuint hHandle);

        template<>
        inline void Bind<gl::VBO>(GLuint hVBO) {
            glBindBuffer(GL_ARRAY_BUFFER, hVBO);
        }

        template<>
        inline void Bind<gl::VAO>(GLuint hVAO) {
            glBindVertexArray(hVAO);
        }

        template<>
        inline void Bind<gl::Texture2D>(GLuint hTex) {
            glBindTexture(GL_TEXTURE_2D, hTex);
        }

        template<typename T>
        void GenerateMipmaps();

        template<>
        inline void GenerateMipmaps<gl::Texture2D>() {
            glGenerateMipmap(GL_TEXTURE_2D);
        }


        template<typename T>
        inline void TexImage2DRGB(Format kFmt, unsigned unWidth, unsigned unHeight, Type kType, void const* pData);

        template<>
        inline void TexImage2DRGB<gl::Texture2D>(Format kFmt, unsigned unWidth, unsigned unHeight, Type kType, void const* pData) {
            GLint uiStorageFormat;
            GLenum kDataFormat, kDataType;

            switch (kFmt) {
            case Format::RGB:
                uiStorageFormat = GL_RGB;
                kDataFormat = GL_RGB;
                break;
            case Format::RGBA:
                uiStorageFormat = GL_RGB;
                kDataFormat = GL_RGBA;
                break;
            }

            switch (kType) {
            case Type::UByte:
                kDataType = GL_UNSIGNED_BYTE;
                break;
            default: assert(0);
            }

            glTexImage2D(GL_TEXTURE_2D, 0, uiStorageFormat, unWidth, unHeight, 0, kDataFormat, kDataType, pData);
        }
    }

    template<typename T>
    inline void Bind(T const& res) {
        detail::Bind<typename T::Resource>(res);
    }

    template<typename T>
    inline void GenerateMipmaps(T const& res) {
        detail::Bind(res);
        detail::GenerateMipmaps<typename T::Resource>();
    }

    template<typename T = gl::Texture2D>
    inline void GenerateMipmaps() {
        detail::GenerateMipmaps<T>();
    }

    template<typename T>
    inline void TexImage2DRGB(Format kFmt, unsigned unWidth, unsigned unHeight, Type kType, void const* pData) {
        detail::TexImage2DRGB<T>(kFmt, unWidth, unHeight, kType, pData);
    }

    template<typename T>
    inline void TexImage2DRGB(T const& hTex, Format kFmt, unsigned unWidth, unsigned unHeight, Type kType, void const* pData) {
        Bind(hTex);
        detail::TexImage2DRGB<T>(kFmt, unWidth, unHeight, kType, pData);
    }

    template<typename T>
    inline void Nearest();

    template<>
    inline void Nearest<gl::Texture2D>() {
        glTexParameteri(GL_TEXTURE_2D, GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    inline void BindElementArray(gl::VBO const& vbo) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo);
    }

    inline void UploadArray(gl::VBO const& vbo, GLsizei size, void const* data) {
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
    }

    inline void UploadElementArray(gl::VBO const& vbo, GLsizei size, void const* data) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
    }

    template<typename Shader>
    inline bool CompileShaderFromString(Shader const& shader, char const* pszSource) {
        GLint bSuccess;
        char const* aSources[1] = { pszSource };
        glShaderSource(shader, 1, aSources, NULL);
        glCompileShader(shader);
        glGetShaderiv(shader, GL_COMPILE_STATUS, &bSuccess);

        if (bSuccess == 0) {
            char pchMsgBuf[128];
            glGetShaderInfoLog(shader, 128, NULL, pchMsgBuf);
            printf("CompileShaderFromString failed: %s\n", pchMsgBuf);
        }

        return bSuccess != 0;
    }
}
