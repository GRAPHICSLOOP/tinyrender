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

struct GouraudShader : IShader
{
    Vec3f vertex_normal[3];
    Vec3f vertex_pos[3];
    Vec2f uv[3];

    virtual Vec4f vertex(int iface, int nthvert)
    {
        uv[nthvert] = model->uv(iface, nthvert);
        vertex_normal[nthvert] = model->normal(iface, nthvert);
        vertex_pos[nthvert] = model->vert(iface, nthvert);

        Vec4f ver = embed<4>(model->vert(iface, nthvert));

        ver = Viewport * NDCView * Perspective * CameraView * ModelView * ver;
        float temp = ver[3];
        ver = ver / ver[3];
        ver[3] = temp;
        return ver;
    }

    virtual bool fragment(Vec3f barycentricCoord, TGAColor& color)
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

    virtual bool fragment(Vec3f barycentricCoord, TGAColor& color)
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

    zbuffer.clear();
    modelView(Vec3f(0, 0, 0), Vec3f(0, 0, 0));
    cameraView(Vec3f(0, 0, 4), Vec3f(0, 180, 0));
    perspective(-1, -10.f, 45, 1);
    ndcView(-1, -10.f, 45, 1);
    viewport(width, height);

    for (int i = 0; i < model->nfaces(); i++)
    {
        std::vector<int> face = model->face(i);
        for (int j = 0; j < 3; j++)
        {
            vertex[j] = shader.vertex(i, j);
        }
        triangle(vertex, shader, image, zbuffer);
    }


    image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
    image.write_tga_file("output.tga");
    delete model;
    return 0;
}
