#include <vector>
#include <cmath>
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include "our_gl.h"

float* depthBuffer = NULL;

Matrix ModelView = Matrix::identity();
Matrix CameraView = Matrix::identity();
Matrix ShadowView = Matrix::identity();
Matrix Viewport = Matrix::identity();
Matrix Perspective = Matrix::identity();
Matrix NDCView = Matrix::identity();
Matrix Orthographic = Matrix::identity();

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);
const TGAColor block = TGAColor(0, 0, 0, 255);

Model* model = NULL;
const int width = 800;
const int height = 800;

Vec3f lightDir(0, 0, -1);

struct sampleShader : IShader
{
    Vec2f uv[3];

    virtual Vec4f vertex(int iface, int nthvert)
    {
        Vec4f ver = embed<4>(model->vert(iface, nthvert));
        uv[nthvert] = model->uv(iface, nthvert);

        ver = Viewport * NDCView * Perspective * CameraView * ModelView * ver;
        float temp = ver[3];
        ver = ver / temp;
        ver[3] = temp;
        return ver;
    }

    virtual bool fragment(Vec2i index, Vec3f barycentricCoord, TGAColor& color)
    {
        float zn = 1 / (barycentricCoord[0] + barycentricCoord[1] + barycentricCoord[2]);
        Vec2f bar_uv = (uv[0] * barycentricCoord[0] + uv[1] * barycentricCoord[1] + uv[2] * barycentricCoord[2]) * zn;
        color = model->diffuse(bar_uv);
        return true;
    }

};

struct GouraudShader : IShader
{
    Vec3f vertex_normal[3];
    Vec3f vertex_pos[3];
    Vec2f uv[3];

    virtual Vec4f vertex(int iface, int nthvert)
    {
        uv[nthvert] = model->uv(iface, nthvert);
        Vec4f ver = embed<4>(model->vert(iface, nthvert));

        vertex_normal[nthvert] = model->normal(iface, nthvert);
        vertex_pos[nthvert] = model->vert(iface, nthvert);

        ver = Perspective * CameraView * ModelView * ver;
        float temp = ver[3];
        ver = ver / ver[3];
        ver[2] = temp;
        ver = Viewport * NDCView * ver;
        ver[3] = temp;
        return ver;
    }

    virtual bool fragment(Vec2i index, Vec3f barycentricCoord, TGAColor& color)
    {
        float zn = 1 / (barycentricCoord[0] + barycentricCoord[1] + barycentricCoord[2]);
        float light;

        Vec2f bar_uv = (uv[0] * barycentricCoord.x + uv[1] * barycentricCoord.y + uv[2] * barycentricCoord.z) * zn;
        Vec3f bar_normal = proj<3>(ModelView * embed<4>((vertex_normal[0] * barycentricCoord.x + vertex_normal[1] * barycentricCoord.y + vertex_normal[2] * barycentricCoord.z) * zn));
        bar_normal.normalize();

        Vec2f delta_vu1 = uv[2] - uv[0];
        Vec2f delta_vu2 = uv[1] - uv[0];
        Vec3f E1 = vertex_pos[2] - vertex_pos[0];
        Vec3f E2 = vertex_pos[1] - vertex_pos[0];

        Vec3f T = (E2 * delta_vu1[0] - E1 * delta_vu2[0]) / (delta_vu1[0] * delta_vu2[1] - delta_vu2[0] * delta_vu1[1]);
        T.normalize();

        T = (T - bar_normal * (T * bar_normal)).normalize();
        Vec3f B = cross(bar_normal, T).normalize();
        mat<3, 3, float> TBN;
        TBN[0] = T;
        TBN[1] = B;
        TBN[2] = bar_normal;

        Vec3f normal_tangent = TBN * model->normal(bar_uv);
        light = std::max(0.f, normal_tangent * (lightDir * -1));

        color = model->diffuse(bar_uv) * light;
        return true;
    }

};

struct ShadowBufferShader : IShader
{
    Vec4f vertex_pos[3];

    virtual Vec4f vertex(int iface, int nthvert)
    {
        Vec4f ver = embed<4>(model->vert(iface, nthvert));
        ver = Viewport * NDCView * Perspective * CameraView * ModelView * ver;
        float temp = ver[3];
        ver = ver / temp;
        ver[3] = temp;

        vertex_pos[nthvert] = ver;
        return ver;
    }

    virtual bool fragment(Vec2i index, Vec3f barycentricCoord, TGAColor& color)
    {
        float zn = 1 / (barycentricCoord[0] + barycentricCoord[1] + barycentricCoord[2]);

        float bar_z = (vertex_pos[0][2] * barycentricCoord.x + vertex_pos[1][2] * barycentricCoord.y + vertex_pos[2][2] * barycentricCoord.z) * zn;

        bar_z = (1 + bar_z);
        color = TGAColor(255, 255, 255, 255) * bar_z;

        return true;
    }

};

struct ShadowShader : IShader
{
    TGAImage* ShadowBuffer;
    Vec4f vertex_pos[3];
    Matrix shadowMatrix;
    Vec2f uv[3];

    virtual Vec4f vertex(int iface, int nthvert)
    {
        Vec4f ver = embed<4>(model->vert(iface, nthvert));
        uv[nthvert] = model->uv(iface, nthvert);
        ver = Viewport * NDCView * Perspective * CameraView * ModelView * ver;
        float temp = ver[3];
        ver = ver / temp;
        ver[3] = temp;


        //Vec4f m = ver;
        //m = m*temp;
        //m[3] = temp;
        //Matrix M = Viewport * NDCView * Perspective * CameraView * ModelView;
        //Vec4f mm = M * M.invert() * m;
        //mm = mm / temp;
        //mm[3] = temp;

        vertex_pos[nthvert] = ver;
        return ver;
    }

    virtual bool fragment(Vec2i index, Vec3f barycentricCoord, TGAColor& color)
    {
        float zn = 1 / (barycentricCoord[0] + barycentricCoord[1] + barycentricCoord[2]);
        Vec2f bar_uv = (uv[0] * barycentricCoord[0] + uv[1] * barycentricCoord[1] + uv[2] * barycentricCoord[2]) * zn;
        

        Vec4f screenFragment = (vertex_pos[0] * barycentricCoord[0] + vertex_pos[1] * barycentricCoord[1] + vertex_pos[2] * barycentricCoord[2]) * zn;
        float temp = screenFragment[3];
        screenFragment = screenFragment * temp;
        screenFragment[3] = temp;

        Vec4f shadowFragment = shadowMatrix * screenFragment;
        temp = shadowFragment[3];
        shadowFragment = shadowFragment / temp;
        shadowFragment[3] = temp;

        unsigned char zbuffer = ((1 + shadowFragment[2]) * 255);
        unsigned char shadowbufferzbuffer = ShadowBuffer->get(shadowFragment[0], shadowFragment[1])[0];

        bool isShadow = zbuffer  < shadowbufferzbuffer * 0.98;
        if (isShadow)
        {
            color = model->diffuse(bar_uv) * 0.5;
            return true;
        }
        color = model->diffuse(bar_uv);
        return true;
    }
};

struct flootShader : IShader
{
    Vec2f uv[3];

    virtual Vec4f vertex(int iface, int nthvert)
    {
        uv[nthvert] = model->uv(iface, nthvert);
        Vec4f ver = embed<4>(model->vert(iface, nthvert));

        ver = Perspective * CameraView * ModelView * ver;
        float temp = ver[3];
        ver = ver / ver[3];
        ver[2] = temp;
        ver = NDCView * ver;
        ver = Viewport * ver;
        ver[3] = temp;
        return ver;
    }

    virtual bool fragment(Vec2i index, Vec3f barycentricCoord, TGAColor& color)
    {
        float zn = 1 / (barycentricCoord[0] + barycentricCoord[1] + barycentricCoord[2]);
        Vec2f bar_uv = (uv[0] * barycentricCoord.x + uv[1] * barycentricCoord.y + uv[2] * barycentricCoord.z) * zn;
        color = model->diffuse(bar_uv);
        return true;
    }
};

struct triangle_s
{
    triangle_s() {};
    triangle_s(Vec3f _0, Vec3f _1, Vec3f _2) : p0(_0), p1(_1), p2(_2) {};
    Vec3f vec[3];
    Vec2f uv[3];

    Vec3f& p0 = vec[0];
    Vec3f& p1 = vec[1];
    Vec3f& p2 = vec[2];
};

void line(int x0, int y0, int x1, int y1, TGAImage& image, TGAColor color)
{
    bool steep = false;
    if (std::abs(x0 - x1) < std::abs(y0 - y1))
    {
        std::swap(x0, y0);
        std::swap(x1, y1);
        steep = true;
    }
    if (x0 > x1) {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }

    int dy = y1 - y0;
    int dx = x1 - x0;

    int error = 0;
    int y = y0;
    int k = 1;

    // 处理 [-1, 0] 范围内的斜率
    if (dy < 0)
    {
        k = -1;
        dy = -dy;
    }

    for (int x = x0; x <= x1; x++)
    {
        if (steep) {
            image.set(y, x, color);
        }
        else {
            image.set(x, y, color);
        }

        error += dy;

        if (error > dx)
        {
            y += k;
            error -= dx;
        }

    }
}

void drawLine(const triangle_s& tri, const TGAColor& color, TGAImage& image)
{
    for (int j = 0; j < 3; j++)
    {
        Vec3f v0 = tri.vec[j];
        Vec3f v1 = tri.vec[(j + 1) % 3];
        line(v0.x, v0.y, v1.x, v1.y, image, white);
    }
}

int main(int argc, char** argv) {
    if (2 == argc) {
        model = new Model(argv[1]);
    }
    else {
        model = new Model("obj/african_head/african_head.obj");
    }

    // draw line
    /*for (int i = 0; i < model->nfaces(); i++) {
        std::vector<int> face = model->face(i);
        for (int j = 0; j < 3; j++)
        {
            tri.vec[j] = world2screen(model->vert(face[j]));
        }
        drawLine(tri, white, image);
    }*/


    TGAImage image(width, height, TGAImage::RGB);
    TGAImage ShaderBuffer(width, height, TGAImage::RGB);
    zbuffer zbuffer(width, height);
    GouraudShader shader;
    lightDir.normalize();
    Vec4f vertex[3];

    ShadowBufferShader ShadowBufferShader;
    zbuffer.clear();
    modelView(Vec3f(0, 0, 0), Vec3f(0, 0, 0));
    cameraView(Vec3f(0, 4, 4), Vec3f(45, 180, 0));
    perspective(-1, -10.f, 45, 1);
    ndcView(-1, -10.f, 45, 1);
    viewport(width, height);

    for (int i = 0; i < model->nfaces(); i++)
    {
        std::vector<int> face = model->face(i);
        for (int j = 0; j < 3; j++)
        {
            vertex[j] = ShadowBufferShader.vertex(i, j);
        }
        triangle(vertex, ShadowBufferShader, ShaderBuffer, zbuffer);
    }
    ShaderBuffer.flip_vertically(); // i want to have the origin at the left bottom corner of the image
    ShaderBuffer.write_tga_file("ShaderBuffer.tga");
    ShaderBuffer.flip_vertically(); // 翻转回去 不然阴影贴图会受到影响
    Matrix ShadowMatrix = Viewport * NDCView * Perspective * CameraView * ModelView;


    zbuffer.clear();
    modelView(Vec3f(0, 0, 0), Vec3f(0, 0, 0));
    cameraView(Vec3f(0, 0, 4), Vec3f(0, 180, 0));
    perspective(-1, -10.f, 45, 1);
    ndcView(-1, -10.f, 45, 1);
    viewport(width, height);

    ShadowShader ShadowShader;
    ShadowShader.ShadowBuffer = &ShaderBuffer;
    ShadowShader.shadowMatrix = ShadowMatrix * ((Viewport * NDCView * Perspective * CameraView * ModelView).invert());
    for (int i = 0; i < model->nfaces(); i++)
    {
        std::vector<int> face = model->face(i);
        for (int j = 0; j < 3; j++)
        {
            vertex[j] = ShadowShader.vertex(i, j);
        }
        triangle(vertex, ShadowShader, image, zbuffer);
    }


    image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
    image.write_tga_file("output.tga");
    delete model;
    return 0;
}
