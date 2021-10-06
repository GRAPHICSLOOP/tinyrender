#pragma once

#include "tgaimage.h"
#include "geometry.h"

extern Matrix ModelView;
extern Matrix CameraView;
extern Matrix ShadowView;
extern Matrix Perspective;
extern Matrix Orthographic;
extern Matrix Viewport;
extern Matrix NDCView;

void modelView(Vec3f location, Vec3f rotation);
void perspective(float near, float far, float fov, float aspect);
void ndcView(float near, float far, float fov, float aspect);
void orthographic(float near, float far, float fov, float aspect, float width);
void viewport(int width, int height);
void cameraView(Vec3f location, Vec3f rotation);

struct IShader
{
    virtual ~IShader() {};
    virtual Vec4f vertex(int iface, int nthvert) = 0;
    virtual bool fragment(Vec3f barycentricCoord, TGAColor& color) = 0;
};

struct zbuffer
{
    zbuffer(Vec2i size)
    {
        this->size = size;
        buffer = new float[size[0] * size[1]];
        for (int i = size[0] * size[1]; i--; buffer[i] = -std::numeric_limits<float>::max());
    }

    zbuffer(int width, int height)
    {
        this->size = Vec2i(width, height);
        buffer = new float[size[0] * size[1]];
        for (int i = size[0] * size[1]; i--; buffer[i] = -std::numeric_limits<float>::max());
    }

    ~zbuffer()
    {
        delete buffer;
    }

    void clear()
    {
        for (int i = size[0] * size[1]; i--; buffer[i] = -std::numeric_limits<float>::max());
    }

    float* buffer;
    Vec2i size;

public:
    float get(int x, int y)
    {
        if (x < 0 || y < 0 || x >= size[0] || y >= size[1]) {
            return std::numeric_limits<float>::max();
        }
        return buffer[x + y * size[0]];
    }

    bool set(int x, int y, float value)
    {
        if (x < 0 || y < 0 || x >= size[0] || y >= size[1]) {
            return false;
        }
        buffer[x + y * size[0]] = value;
        return true;
    }

};

void triangle(const Vec4f* vertex, IShader& shader, TGAImage& image, zbuffer& zbuffer);