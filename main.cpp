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

    virtual bool fragment(Vec3f barycentricCoord, TGAColor& color)
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

    virtual bool fragment(Vec3f barycentricCoord, TGAColor& color)
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

int main(int argc, char** argv) {
    if (2 == argc) {
        model = new Model(argv[1]);
    }
    else {
        model = new Model("obj/african_head/african_head.obj");
    }

    TGAImage image(width, height, TGAImage::RGB);
    TGAImage ShaderBuffer(width, height, TGAImage::RGB);
    zbuffer zbuffer(width, height);
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
