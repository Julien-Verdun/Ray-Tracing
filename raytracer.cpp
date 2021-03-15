#define _CRT_SECURE_NO_WARNINGS 1
#include <vector>

#include <algorithm>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <ctime>
#include <fstream>
#include <iostream>
#define M_PI 3.14159265358979323846

#include <random>

#include <string>
#include <stdio.h>
#include <list>

// canva3D : https://www.cadnav.com/
// free3D : https://free3d.com/fr/

static std::default_random_engine engine(10);                // random seed = 10 // generateur de nombre aleatoire
static std::uniform_real_distribution<double> uniform(0, 1); // generation d'une loi uniforme entre 0 et 1

class Vector
{
public:
    explicit Vector(double x = 0, double y = 0, double z = 0)
    {
        coords[0] = x;
        coords[1] = y;
        coords[2] = z;
    }
    double operator[](int i) const { return coords[i]; };
    double &operator[](int i) { return coords[i]; };
    double sqrNorm()
    {
        return coords[0] * coords[0] + coords[1] * coords[1] + coords[2] * coords[2];
    }
    Vector get_normalized()
    {
        double n = sqrt(sqrNorm());
        return Vector(coords[0] / n, coords[1] / n, coords[2] / n);
    }

    Vector &operator+=(const Vector &a)
    {
        coords[0] += a[0];
        coords[1] += a[1];
        coords[2] += a[2];
        return *this;
    }

private:
    double coords[3];
};

Vector operator+(const Vector &a, const Vector &b)
{
    return Vector(a[0] + b[0], a[1] + b[1], a[2] + b[2]);
}

Vector operator-(const Vector &a, const Vector &b)
{
    return Vector(a[0] - b[0], a[1] - b[1], a[2] - b[2]);
}

Vector operator-(const Vector &a)
{
    return Vector(-a[0], -a[1], -a[2]);
}

Vector operator*(double a, const Vector &b)
{
    return Vector(a * b[0], a * b[1], a * b[2]);
}

Vector operator*(const Vector &a, double b)
{
    return Vector(a[0] * b, a[1] * b, a[2] * b);
}

Vector operator*(const Vector &a, const Vector &b)
{
    return Vector(a[0] * b[0], a[1] * b[1], a[2] * b[2]);
}

Vector operator/(const Vector &a, double b)
{
    return Vector(a[0] / b, a[1] / b, a[2] / b);
}

Vector cross(const Vector &a, const Vector &b)
{
    return Vector(a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]);
}

double dot(const Vector &a, const Vector &b)
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

double sqr(double x)
{
    return x * x;
}

Vector random_cos(const Vector &N)
{
    double u1 = uniform(engine);
    double u2 = uniform(engine);
    double x = cos(2 * M_PI * u1) * sqrt(1 - u2);
    double y = sin(2 * M_PI * u1) * sqrt(1 - u2);
    double z = sqrt(u2);
    Vector T1;
    if (N[0] < N[1] && N[0] < N[2])
    {
        T1 = Vector(0, N[2], -N[1]);
    }
    else
    {
        if (N[1] < N[2] && N[1] < N[0])
        {
            T1 = Vector(N[2], 0, -N[0]);
        }
        else
        {
            T1 = Vector(N[1], -N[0], 0);
        }
    }
    T1 = T1.get_normalized();
    Vector T2 = cross(N, T1);
    return z * N + x * T1 + y * T2;
}

class Ray
{
public:
    Ray(const Vector &C, const Vector &u) : C(C), u(u)
    {
    }

    Vector C, u;
};

class Object
{
public:
    Object(){};
    virtual bool intersect(const Ray &r, Vector &P, Vector &normale, double &t, Vector &color) = 0;

    Vector albedo;
    bool isMirror, isTransparent;
};

class BoudingBox
{
public:
    bool intersect(const Ray &r)
    {
        //intersection avec les plans verticaux
        double t1x = (mini[0] - r.C[0]) / r.u[0],
               t2x = (maxi[0] - r.C[0]) / r.u[0];
        double txMin = std::min(t1x, t2x), txMax = std::max(t1x, t2x);

        //intersection avec les plans horizontaux
        double t1y = (mini[1] - r.C[1]) / r.u[1],
               t2y = (maxi[1] - r.C[1]) / r.u[1];
        double tyMin = std::min(t1y, t2y), tyMax = std::max(t1y, t2y);

        //intersection avec les plans 3eme dimensions
        double t1z = (mini[2] - r.C[2]) / r.u[2],
               t2z = (maxi[2] - r.C[2]) / r.u[2];
        double tzMin = std::min(t1z, t2z), tzMax = std::max(t1z, t2z);

        //max et min des min et max
        double tMax = std::min(txMax, std::min(tyMax, tzMax)),
               tMin = std::max(txMin, std::max(tyMin, tzMin));
        if (tMax < 0)
            return false;
        return tMax > tMin;
    }
    Vector mini, maxi;
};

class Noeud
{
public:
    Noeud *fg, *fd;
    BoudingBox b;
    int debut, fin;
};

class TriangleIndices
{
public:
    TriangleIndices(int vtxi = -1, int vtxj = -1, int vtxk = -1, int ni = -1, int nj = -1, int nk = -1, int uvi = -1, int uvj = -1, int uvk = -1, int group = -1, bool added = false) : vtxi(vtxi), vtxj(vtxj), vtxk(vtxk), uvi(uvi), uvj(uvj), uvk(uvk), ni(ni), nj(nj), nk(nk), group(group){};
    int vtxi, vtxj, vtxk; // indices within the vertex coordinates array
    int uvi, uvj, uvk;    // indices within the uv coordinates array
    int ni, nj, nk;       // indices within the normals array
    int group;            // face group
};

class TriangleMesh : public Object
{
public:
    ~TriangleMesh() {}
    TriangleMesh(const Vector &albedo, bool mirror = false, bool transp = false)
    {
        this->albedo = albedo;
        isMirror = mirror;
        isTransparent = transp;
        BVH = new Noeud;
    };

    BoudingBox buildBB(int debut, int fin) // debut et fin : indice de TRIANGLES
    {
        BoudingBox bb;
        bb.mini = Vector(1E9, 1E9, 1E9);
        bb.maxi = Vector(-1E9, -1E9, -1E9);
        // for (int i = 0; i < vertices.size(); i++)
        // {
        for (int i = debut; i < fin; i++)
        {
            for (int j = 0; j < 3; j++)
            {
                // bb.mini[j] = std::min(bb.mini[j], vertices[i][j]);
                // bb.maxi[j] = std::max(bb.maxi[j], vertices[i][j]);
                bb.mini[j] = std::min(bb.mini[j], vertices[indices[i].vtxi][j]);
                bb.maxi[j] = std::max(bb.maxi[j], vertices[indices[i].vtxi][j]);
                bb.mini[j] = std::min(bb.mini[j], vertices[indices[i].vtxj][j]);
                bb.maxi[j] = std::max(bb.maxi[j], vertices[indices[i].vtxj][j]);
                bb.mini[j] = std::min(bb.mini[j], vertices[indices[i].vtxk][j]);
                bb.maxi[j] = std::max(bb.maxi[j], vertices[indices[i].vtxk][j]);
            }
        }
        return bb;
    }

    void buildBVH(Noeud *n, int debut, int fin)
    {
        n->debut = debut;
        n->fin = fin;
        n->b = buildBB(n->debut, n->fin);

        Vector diag = n->b.maxi - n->b.mini;
        int dim; // dimension a diviser en 2
        if (diag[0] >= diag[1] && diag[0] >= diag[2])
        {
            dim = 0;
        }
        else
        {
            if (diag[1] >= diag[0] && diag[1] >= diag[2])
            {
                dim = 1;
            }
            else
            {
                dim = 2;
            }
        }

        double milieu = (n->b.mini[dim] + n->b.maxi[dim]) * 0.5; //milieu de la boite englobante selon la direction dim
        int indice_pivot = n->debut;
        for (int i = n->debut; i < n->fin; i++)
        {
            double milieu_triangle = (vertices[indices[i].vtxi][dim] + vertices[indices[i].vtxj][dim] + vertices[indices[i].vtxk][dim]) / 3;
            if (milieu_triangle < milieu)
            {
                std::swap(indices[i], indices[indice_pivot]);
                indice_pivot++;
            }
        }

        n->fg = NULL;
        n->fd = NULL;

        if (indice_pivot == debut || indice_pivot == fin || fin - debut < 5)
            return;

        n->fg = new Noeud;
        n->fd = new Noeud;

        buildBVH(n->fg, n->debut, indice_pivot);
        buildBVH(n->fd, indice_pivot, n->fin);
    }

    void readOBJ(const char *obj)
    {

        char matfile[255];
        char grp[255];

        FILE *f;
        f = fopen(obj, "r");
        int curGroup = -1;
        while (!feof(f))
        {
            char line[255];
            if (!fgets(line, 255, f))
                break;

            std::string linetrim(line);
            linetrim.erase(linetrim.find_last_not_of(" \r\t") + 1);
            strcpy(line, linetrim.c_str());

            if (line[0] == 'u' && line[1] == 's')
            {
                sscanf(line, "usemtl %[^\n]\n", grp);
                curGroup++;
            }

            if (line[0] == 'v' && line[1] == ' ')
            {
                Vector vec;

                Vector col;
                if (sscanf(line, "v %lf %lf %lf %lf %lf %lf\n", &vec[0], &vec[1], &vec[2], &col[0], &col[1], &col[2]) == 6)
                {
                    col[0] = std::min(1., std::max(0., col[0]));
                    col[1] = std::min(1., std::max(0., col[1]));
                    col[2] = std::min(1., std::max(0., col[2]));

                    vertices.push_back(vec);
                    vertexcolors.push_back(col);
                }
                else
                {
                    sscanf(line, "v %lf %lf %lf\n", &vec[0], &vec[1], &vec[2]);
                    vertices.push_back(vec);
                }
            }
            if (line[0] == 'v' && line[1] == 'n')
            {
                Vector vec;
                sscanf(line, "vn %lf %lf %lf\n", &vec[0], &vec[1], &vec[2]);
                normals.push_back(vec);
            }
            if (line[0] == 'v' && line[1] == 't')
            {
                Vector vec;
                sscanf(line, "vt %lf %lf\n", &vec[0], &vec[1]);
                uvs.push_back(vec);
            }
            if (line[0] == 'f')
            {
                TriangleIndices t;
                int i0, i1, i2, i3;
                int j0, j1, j2, j3;
                int k0, k1, k2, k3;
                int nn;
                t.group = curGroup;

                char *consumedline = line + 1;
                int offset;

                nn = sscanf(consumedline, "%u/%u/%u %u/%u/%u %u/%u/%u%n", &i0, &j0, &k0, &i1, &j1, &k1, &i2, &j2, &k2, &offset);
                if (nn == 9)
                {
                    if (i0 < 0)
                        t.vtxi = vertices.size() + i0;
                    else
                        t.vtxi = i0 - 1;
                    if (i1 < 0)
                        t.vtxj = vertices.size() + i1;
                    else
                        t.vtxj = i1 - 1;
                    if (i2 < 0)
                        t.vtxk = vertices.size() + i2;
                    else
                        t.vtxk = i2 - 1;
                    if (j0 < 0)
                        t.uvi = uvs.size() + j0;
                    else
                        t.uvi = j0 - 1;
                    if (j1 < 0)
                        t.uvj = uvs.size() + j1;
                    else
                        t.uvj = j1 - 1;
                    if (j2 < 0)
                        t.uvk = uvs.size() + j2;
                    else
                        t.uvk = j2 - 1;
                    if (k0 < 0)
                        t.ni = normals.size() + k0;
                    else
                        t.ni = k0 - 1;
                    if (k1 < 0)
                        t.nj = normals.size() + k1;
                    else
                        t.nj = k1 - 1;
                    if (k2 < 0)
                        t.nk = normals.size() + k2;
                    else
                        t.nk = k2 - 1;
                    indices.push_back(t);
                }
                else
                {
                    nn = sscanf(consumedline, "%u/%u %u/%u %u/%u%n", &i0, &j0, &i1, &j1, &i2, &j2, &offset);
                    if (nn == 6)
                    {
                        if (i0 < 0)
                            t.vtxi = vertices.size() + i0;
                        else
                            t.vtxi = i0 - 1;
                        if (i1 < 0)
                            t.vtxj = vertices.size() + i1;
                        else
                            t.vtxj = i1 - 1;
                        if (i2 < 0)
                            t.vtxk = vertices.size() + i2;
                        else
                            t.vtxk = i2 - 1;
                        if (j0 < 0)
                            t.uvi = uvs.size() + j0;
                        else
                            t.uvi = j0 - 1;
                        if (j1 < 0)
                            t.uvj = uvs.size() + j1;
                        else
                            t.uvj = j1 - 1;
                        if (j2 < 0)
                            t.uvk = uvs.size() + j2;
                        else
                            t.uvk = j2 - 1;
                        indices.push_back(t);
                    }
                    else
                    {
                        nn = sscanf(consumedline, "%u %u %u%n", &i0, &i1, &i2, &offset);
                        if (nn == 3)
                        {
                            if (i0 < 0)
                                t.vtxi = vertices.size() + i0;
                            else
                                t.vtxi = i0 - 1;
                            if (i1 < 0)
                                t.vtxj = vertices.size() + i1;
                            else
                                t.vtxj = i1 - 1;
                            if (i2 < 0)
                                t.vtxk = vertices.size() + i2;
                            else
                                t.vtxk = i2 - 1;
                            indices.push_back(t);
                        }
                        else
                        {
                            nn = sscanf(consumedline, "%u//%u %u//%u %u//%u%n", &i0, &k0, &i1, &k1, &i2, &k2, &offset);
                            if (i0 < 0)
                                t.vtxi = vertices.size() + i0;
                            else
                                t.vtxi = i0 - 1;
                            if (i1 < 0)
                                t.vtxj = vertices.size() + i1;
                            else
                                t.vtxj = i1 - 1;
                            if (i2 < 0)
                                t.vtxk = vertices.size() + i2;
                            else
                                t.vtxk = i2 - 1;
                            if (k0 < 0)
                                t.ni = normals.size() + k0;
                            else
                                t.ni = k0 - 1;
                            if (k1 < 0)
                                t.nj = normals.size() + k1;
                            else
                                t.nj = k1 - 1;
                            if (k2 < 0)
                                t.nk = normals.size() + k2;
                            else
                                t.nk = k2 - 1;
                            indices.push_back(t);
                        }
                    }
                }

                consumedline = consumedline + offset;

                while (true)
                {
                    if (consumedline[0] == '\n')
                        break;
                    if (consumedline[0] == '\0')
                        break;
                    nn = sscanf(consumedline, "%u/%u/%u%n", &i3, &j3, &k3, &offset);
                    TriangleIndices t2;
                    t2.group = curGroup;
                    if (nn == 3)
                    {
                        if (i0 < 0)
                            t2.vtxi = vertices.size() + i0;
                        else
                            t2.vtxi = i0 - 1;
                        if (i2 < 0)
                            t2.vtxj = vertices.size() + i2;
                        else
                            t2.vtxj = i2 - 1;
                        if (i3 < 0)
                            t2.vtxk = vertices.size() + i3;
                        else
                            t2.vtxk = i3 - 1;
                        if (j0 < 0)
                            t2.uvi = uvs.size() + j0;
                        else
                            t2.uvi = j0 - 1;
                        if (j2 < 0)
                            t2.uvj = uvs.size() + j2;
                        else
                            t2.uvj = j2 - 1;
                        if (j3 < 0)
                            t2.uvk = uvs.size() + j3;
                        else
                            t2.uvk = j3 - 1;
                        if (k0 < 0)
                            t2.ni = normals.size() + k0;
                        else
                            t2.ni = k0 - 1;
                        if (k2 < 0)
                            t2.nj = normals.size() + k2;
                        else
                            t2.nj = k2 - 1;
                        if (k3 < 0)
                            t2.nk = normals.size() + k3;
                        else
                            t2.nk = k3 - 1;
                        indices.push_back(t2);
                        consumedline = consumedline + offset;
                        i2 = i3;
                        j2 = j3;
                        k2 = k3;
                    }
                    else
                    {
                        nn = sscanf(consumedline, "%u/%u%n", &i3, &j3, &offset);
                        if (nn == 2)
                        {
                            if (i0 < 0)
                                t2.vtxi = vertices.size() + i0;
                            else
                                t2.vtxi = i0 - 1;
                            if (i2 < 0)
                                t2.vtxj = vertices.size() + i2;
                            else
                                t2.vtxj = i2 - 1;
                            if (i3 < 0)
                                t2.vtxk = vertices.size() + i3;
                            else
                                t2.vtxk = i3 - 1;
                            if (j0 < 0)
                                t2.uvi = uvs.size() + j0;
                            else
                                t2.uvi = j0 - 1;
                            if (j2 < 0)
                                t2.uvj = uvs.size() + j2;
                            else
                                t2.uvj = j2 - 1;
                            if (j3 < 0)
                                t2.uvk = uvs.size() + j3;
                            else
                                t2.uvk = j3 - 1;
                            consumedline = consumedline + offset;
                            i2 = i3;
                            j2 = j3;
                            indices.push_back(t2);
                        }
                        else
                        {
                            nn = sscanf(consumedline, "%u//%u%n", &i3, &k3, &offset);
                            if (nn == 2)
                            {
                                if (i0 < 0)
                                    t2.vtxi = vertices.size() + i0;
                                else
                                    t2.vtxi = i0 - 1;
                                if (i2 < 0)
                                    t2.vtxj = vertices.size() + i2;
                                else
                                    t2.vtxj = i2 - 1;
                                if (i3 < 0)
                                    t2.vtxk = vertices.size() + i3;
                                else
                                    t2.vtxk = i3 - 1;
                                if (k0 < 0)
                                    t2.ni = normals.size() + k0;
                                else
                                    t2.ni = k0 - 1;
                                if (k2 < 0)
                                    t2.nj = normals.size() + k2;
                                else
                                    t2.nj = k2 - 1;
                                if (k3 < 0)
                                    t2.nk = normals.size() + k3;
                                else
                                    t2.nk = k3 - 1;
                                consumedline = consumedline + offset;
                                i2 = i3;
                                k2 = k3;
                                indices.push_back(t2);
                            }
                            else
                            {
                                nn = sscanf(consumedline, "%u%n", &i3, &offset);
                                if (nn == 1)
                                {
                                    if (i0 < 0)
                                        t2.vtxi = vertices.size() + i0;
                                    else
                                        t2.vtxi = i0 - 1;
                                    if (i2 < 0)
                                        t2.vtxj = vertices.size() + i2;
                                    else
                                        t2.vtxj = i2 - 1;
                                    if (i3 < 0)
                                        t2.vtxk = vertices.size() + i3;
                                    else
                                        t2.vtxk = i3 - 1;
                                    consumedline = consumedline + offset;
                                    i2 = i3;
                                    indices.push_back(t2);
                                }
                                else
                                {
                                    consumedline = consumedline + 1;
                                }
                            }
                        }
                    }
                }
            }
        }
        fclose(f);
    }

    bool intersect(const Ray &r, Vector &P, Vector &normale, double &t, Vector &color)
    {
        if (!BVH->b.intersect(r))
            return false;

        t = 1E9;
        bool has_inter = false;

        std::list<Noeud *> l;
        l.push_back(BVH);
        while (!l.empty())
        {
            Noeud *c = l.front();
            l.pop_front();
            if (c->fg)
            {
                if (c->fg->b.intersect(r))
                {
                    l.push_front(c->fg);
                }
                if (c->fd->b.intersect(r))
                {
                    l.push_front(c->fd);
                }
            }
            else
            {

                for (int i = c->debut; i < c->fin; i++)
                {
                    // calcul d'intersection
                    const Vector &A = vertices[indices[i].vtxi],
                                 &B = vertices[indices[i].vtxj],
                                 &C = vertices[indices[i].vtxk];
                    Vector e1 = B - A,
                           e2 = C - A,
                           N = cross(e1, e2),
                           AO = r.C - A,
                           AOu = cross(AO, r.u);
                    double invUN = 1. / dot(r.u, N);

                    double beta = -dot(e2, AOu) * invUN,
                           gamma = dot(e1, AOu) * invUN,
                           alpha = 1 - beta - gamma,
                           localt = -dot(AO, N) * invUN;
                    if (beta >= 0 && gamma >= 0 && beta <= 1 && gamma <= 1 && alpha >= 0 && localt > 0)
                    {
                        has_inter = true;
                        if (localt < t)
                        {
                            t = localt;
                            // normale = N.get_normalized();
                            normale = alpha * normals[indices[i].ni] + beta * normals[indices[i].nj] + gamma * normals[indices[i].nk];
                            normale = normale.get_normalized();
                            P = r.C + r.u;
                            if (textures.size() > 0)
                            {

                                int H = Htex[indices[i].group],
                                    W = Wtex[indices[i].group];
                                Vector UV = alpha * uvs[indices[i].uvi] + beta * uvs[indices[i].uvj] + gamma * uvs[indices[i].uvk];
                                UV = UV * Vector(W, H, 0);
                                int uvx = UV[0] + 0.5;
                                int uvy = UV[1] + 0.5;
                                uvx = uvx % W;
                                uvy = uvy % H;
                                if (uvx < 0)
                                    uvx += W;
                                if (uvy < 0)
                                    uvy += H;
                                uvy = H - uvy - 1;
                                color = Vector(std::pow(textures[indices[i].group][(uvy * W + uvx) * 3] / 255., 2.2),
                                               std::pow(textures[indices[i].group][(uvy * W + uvx) * 3 + 1] / 255., 2.2),
                                               std::pow(textures[indices[i].group][(uvy * W + uvx) * 3 + 2] / 255., 2.2));
                            }
                            else
                            {
                                color = albedo; // Vector(1., 1., 1.);
                            }
                        }
                    }
                }
            }
        }

        // for (int i = 0; i < indices.size(); i++)
        // {
        //     // calcul d'intersection
        //     const Vector &A = vertices[indices[i].vtxi],
        //                  &B = vertices[indices[i].vtxj],
        //                  &C = vertices[indices[i].vtxk];
        //     Vector e1 = B - A,
        //            e2 = C - A,
        //            N = cross(e1, e2),
        //            AO = r.C - A,
        //            AOu = cross(AO, r.u);
        //     double invUN = 1. / dot(r.u, N);

        //     double beta = -dot(e2, AOu) * invUN,
        //            gamma = dot(e1, AOu) * invUN,
        //            alpha = 1 - beta - gamma,
        //            localt = -dot(AO, N) * invUN;
        //     if (beta >= 0 && gamma >= 0 && beta <= 1 && gamma <= 1 && alpha >= 0 && localt > 0)
        //     {
        //         has_inter = true;
        //         if (localt < t)
        //         {
        //             t = localt;
        //             normale = N.get_normalized();
        //             P = r.C + r.u;
        //         }
        //     }
        // }

        return has_inter;
    };

    void loadTexture(const char *filename)
    {
        int W, H, C;
        unsigned char *texture = stbi_load(filename, &W, &H, &C, 3);
        Wtex.push_back(W);
        Htex.push_back(H);
        textures.push_back(texture);
    }

    void invertNormals()
    {
        for (int i = 0; i <= vertices.size(); i++)
        {
            normals[i] = -normals[i];
        }
    }

    void translate(int dir, int dist)
    {
        if (dist != 0)
        {

            for (int i = 0; i <= vertices.size(); i++)
            {
                vertices[i][dir] += dist;
            }
        }
        else
        {

            for (int i = 0; i <= vertices.size(); i++)
            {
                vertices[i][dir] = -vertices[i][dir];
            }
        }
    }
    void rotateNormals(int axe1, int axe2)
    {
        for (int i = 0; i <= vertices.size(); i++)
        {
            std::swap(normals[i][axe1], normals[i][axe2]);
        }
    }
    void rotate(int axe1, int axe2)
    {
        for (int i = 0; i <= vertices.size(); i++)
        {
            std::swap(vertices[i][axe1], vertices[i][axe2]);
        }
    }

    void rotateAroundAxe(int axe, double angle)
    {
        int axe1, axe2;
        if (axe == 2)
        {
            axe1 = 0;
            axe2 = 1;
        }
        else
        {
            if (axe == 1)
            {
                axe1 = 0;
                axe2 = 2;
            }
            else
            {
                axe1 = 1;
                axe2 = 2;
            }
        }
        int axe1value;
        for (int i = 0; i <= vertices.size(); i++)
        {
            axe1value = vertices[i][axe1];
            vertices[i][axe1] = cos(angle * M_PI / 180) * axe1value - sin(angle * M_PI / 180) * vertices[i][axe2];
            vertices[i][axe2] = sin(angle * M_PI / 180) * axe1value + cos(angle * M_PI / 180) * vertices[i][axe2];
        }
    }

    void zoom(int zoomFacteur)
    {
        int meanX = 0, meanY = 0, meanZ = 0;
        for (int i = 0; i <= vertices.size(); i++)
        {
            meanX += vertices[i][0];
            meanY += vertices[i][1];
            meanZ += vertices[i][2];
        }
        meanX /= meanX;
        meanY /= meanY;
        meanZ /= meanZ;
        Vector center = Vector(meanX, meanY, meanZ);

        for (int i = 0; i <= vertices.size(); i++)
        {
            vertices[i][0] = vertices[i][0] + zoomFacteur * (center * vertices[i])[0];
            vertices[i][1] = vertices[i][1] + zoomFacteur * (center * vertices[i])[1];
            vertices[i][2] = vertices[i][2] + zoomFacteur * (center * vertices[i])[2];
        }
    }

    std::vector<TriangleIndices> indices;
    std::vector<Vector> vertices;
    std::vector<Vector> normals;
    std::vector<Vector> uvs;
    std::vector<Vector> vertexcolors;
    std::vector<unsigned char *> textures;
    std::vector<int> Wtex, Htex;
    BoudingBox bb;

    Noeud *BVH;
};

class Sphere : public Object
{
public:
    Sphere(const Vector &O, double R, const Vector &albedo, bool isMirror = false, bool isTransparent = false) : O(O), R(R)
    {
        this->albedo = albedo;
        this->isMirror = isMirror;
        this->isTransparent = isTransparent;
    }
    bool intersect(const Ray &r, Vector &P, Vector &N, double &t, Vector &color)
    {
        // solves a*t^2 + b*t + c = 0
        double a = 1;
        double b = 2 * dot(r.u, r.C - O);
        double c = (r.C - O).sqrNorm() - R * R;
        double delta = b * b - 4 * a * c;
        color = this->albedo;

        if (delta < 0)
        {
            return false;
        }
        double sqDelta = sqrt(delta);
        double t2 = (-b + sqDelta) / (2 * a);

        if (t2 < 0)
            return false;

        // double t;
        double t1 = (-b - sqDelta) / (2 * a);

        if (t1 > 0)
        {
            t = t1;
        }
        else
        {
            t = t2;
        }

        P = r.C + t * r.u;
        N = (P - O).get_normalized();

        return true;
    }
    Vector O;
    double R;
};

class Scene
{
public:
    Scene(){};
    std::vector<Object *> objects;
    Vector L;
    double I;
    bool intersect(const Ray &r, Vector &P, Vector &N, Vector &albedo, bool &mirror, bool &transp, double &t, int &objectId)
    {
        t = 1E10;
        bool has_inter = false;
        for (int i = 0; i < objects.size(); i++)
        {
            Vector localP, localN, localAlbedo;
            double localt;
            if (objects[i]->intersect(r, localP, localN, localt, localAlbedo) && localt < t)
            {
                t = localt;
                has_inter = true;
                albedo = localAlbedo;
                P = localP;
                N = localN;
                mirror = objects[i]->isMirror;
                transp = objects[i]->isTransparent;
                objectId = i;
            }
        }
        return has_inter;
    };

    Vector getColor(const Ray &r, int rebond, bool lastDiffuse)
    {
        double epsilon = 0.00001;
        Vector P, N, albedo;
        double t;
        bool mirror, transp;
        int objectId;
        bool inter = intersect(r, P, N, albedo, mirror, transp, t, objectId);
        Vector color(0, 0, 0);
        if (rebond > 10) //5 normalement
            return Vector(0., 0., 0.);
        if (inter)
        {
            if (objectId == 0)
            {
                if (rebond == 0 || !lastDiffuse)
                {
                    return Vector(I, I, I) / (4 * M_PI * M_PI * sqr(dynamic_cast<Sphere *>(objects[0])->R));
                }
                return Vector(0., 0., 0.);
            }
            if (mirror)
            {
                Vector reflectedDir = r.u - 2 * dot(r.u, N) * N;
                Ray reflectedRay(P + epsilon * N, reflectedDir);
                return getColor(reflectedRay, rebond + 1, false);
            }
            else
            {
                if (transp)
                {
                    double n1 = 1, n2 = 1.4;
                    Vector N2 = N;
                    if (dot(r.u, N) > 0)
                    { //on sort de la sphère
                        std::swap(n1, n2);
                        N2 = -N;
                    }

                    double rad = 1 - sqr(n1 / n2) * (1 - sqr(dot(r.u, N2)));

                    if (rad < 0) //rayon rasant, i-e rayon reflechie uniquement
                    {
                        Vector reflectedDir = r.u - 2 * dot(r.u, N) * N;
                        Ray reflectedRay(P + epsilon * N, reflectedDir);
                        return getColor(reflectedRay, rebond + 1, false);
                    }

                    double k0 = sqr(n1 - n2) / sqr(n1 + n2);

                    double R = k0 + (1 - k0) * sqr(sqr(1 - std::abs(dot(N2, r.u)))) * (1 - std::abs(dot(N2, r.u)));

                    // implementation de la transmission de Fresnel avec tirage aleatoire et moyenne
                    double random = rand() % 100;

                    if (random <= 100 * R)
                    {
                        Vector reflectedDir = r.u - 2 * dot(r.u, N) * N;
                        Ray reflectedRay(P + epsilon * N, reflectedDir);
                        return getColor(reflectedRay, rebond + 1, false);
                    }
                    else
                    {
                        // double T = 1 - R;
                        Vector tT = n1 / n2 * (r.u - dot(r.u, N2) * N2);
                        Vector tN = -sqrt(rad) * N2;
                        Vector refractedDir = tT + tN;
                        return getColor(Ray(P - epsilon * N2, refractedDir), rebond + 1, false);
                    }

                    // Vector reflectedDir = r.u - 2 * dot(r.u, N) * N;
                    // Ray reflectedRay(P + epsilon * N, reflectedDir);

                    // double T = 1 - R;

                    // Vector tT = n1 / n2 * (r.u - dot(r.u, N2) * N2);
                    // Vector tN = -sqrt(rad) * N2;
                    // Vector refractedDir = tT + tN;

                    // // sans transmission de Fresnel
                    // // return getColor(Ray(P - epsilon * N2, refractedDir), rebond + 1);

                    // // avec transmission de Fresnel
                    // return (T * getColor(Ray(P - epsilon * N2, refractedDir), rebond + 1) + R * getColor(reflectedRay, rebond + 1));
                }
                else //eclairage direct
                {
                    // Vector PL = L - P;
                    // double d = sqrt(PL.sqrNorm());
                    // Vector shadowP, shadowN, shadowAlbedo;
                    // double shadowt;
                    // int objectId;
                    // bool shadowMirror, shadowTransp;
                    // Ray shadowRay(P + 0.00001 * N, PL / d);
                    // bool shadowInter = intersect(shadowRay, shadowP, shadowN, shadowAlbedo, shadowMirror, shadowTransp, shadowt, objectId);
                    // if (shadowInter && shadowt < d)
                    // {
                    //     color = Vector(0., 0., 0.);
                    // }
                    // else
                    // {
                    //     color = (I / (4 * M_PI * d * d)) * (albedo / M_PI) * std::max(0., dot(N, PL / d));
                    // }

                    // eclairage direct

                    Vector PL = L - P;
                    PL = PL.get_normalized();
                    Vector w = random_cos(-PL);
                    Vector xprime = w * dynamic_cast<Sphere *>(objects[0])->R + dynamic_cast<Sphere *>(objects[0])->O;
                    Vector Pxprime = xprime - P;
                    double d = sqrt(Pxprime.sqrNorm());
                    Pxprime = Pxprime / d;

                    Vector shadowP, shadowN, shadowAlbedo;
                    double shadowt;
                    int objectId;
                    bool shadowMirror, shadowTransp;
                    Ray shadowRay(P + 0.00001 * N, Pxprime);
                    bool shadowInter = intersect(shadowRay, shadowP, shadowN, shadowAlbedo, shadowMirror, shadowTransp, shadowt, objectId);
                    if (shadowInter && shadowt < d - 0.0001)
                    {
                        color = Vector(0., 0., 0.);
                    }
                    else
                    {
                        double R2 = sqr(dynamic_cast<Sphere *>(objects[0])->R);
                        double proba = std::max(1E-8, dot(-PL, w)) / (M_PI * R2);
                        double J = std::max(0., dot(w, -Pxprime)) / (d * d);
                        color = (I / (4 * M_PI * M_PI * R2)) * (albedo / M_PI) * std::max(0., dot(N, Pxprime)) * J / proba;
                    }

                    // eclairage indirect
                    Vector wi = random_cos(N);
                    Ray wiRay(P + epsilon * N, wi);
                    color += albedo * getColor(wiRay, rebond + 1, true);
                }
            }
        }
        return color;
    }
};

void integrateCos()
{
    int N = 10000;
    double sigma = 0.25;
    double s = 0;
    for (int i = 0; i < N; i++)
    {
        double u1 = uniform(engine), u2 = uniform(engine);
        double xi = sigma * cos(2 * M_PI * u1) * sqrt(-2 * log(u2));
        if (xi >= -M_PI / 2 && xi <= M_PI / 2)
        {
            double p = 1 / (sigma * sqrt(2 * M_PI)) * exp(-xi * xi / (2 * sigma * sigma));
            s += pow(cos(xi), 10) / p / N;
        }
    }

    std::ofstream execution_file;
    execution_file.open("integral.txt");
    execution_file << s;
    execution_file.close();

    // std::cout << s << std::endl;
}

void integrate4D()
{
    int N = 100000;
    double sigma = 1;
    double s = 0;
    for (int i = 0; i < N; i++)
    {
        double u1 = uniform(engine), u2 = uniform(engine);
        double x1 = sigma * cos(2 * M_PI * u1) * sqrt(-2 * log(u2)),
               x2 = sigma * sin(2 * M_PI * u1) * sqrt(-2 * log(u2));

        double u3 = uniform(engine), u4 = uniform(engine);
        double x3 = sigma * cos(2 * M_PI * u3) * sqrt(-2 * log(u4)),
               x4 = sigma * sin(2 * M_PI * u3) * sqrt(-2 * log(u4));

        if ((x1 >= -M_PI / 2 && x1 <= M_PI / 2) && (x2 >= -M_PI / 2 && x2 <= M_PI / 2) && (x3 >= -M_PI / 2 && x3 <= M_PI / 2) && (x4 >= -M_PI / 2 && x4 <= M_PI / 2))
        {
            double p1 = 1 / (sigma * sqrt(2 * M_PI)) * exp(-x1 * x1 / (2 * sigma * sigma)),
                   p2 = 1 / (sigma * sqrt(2 * M_PI)) * exp(-x2 * x2 / (2 * sigma * sigma)),
                   p3 = 1 / (sigma * sqrt(2 * M_PI)) * exp(-x3 * x3 / (2 * sigma * sigma)),
                   p4 = 1 / (sigma * sqrt(2 * M_PI)) * exp(-x4 * x4 / (2 * sigma * sigma));
            s += pow(cos(x1 + x2 + x3 + x4), 2) / (p1 * p2 * p3 * p4) / N;
        }
    }

    std::ofstream execution_file;
    execution_file.open("integral4D.txt");
    execution_file << s;
    execution_file.close();

    // std::cout << s << std::endl;
}

int main()
{
    float ini_time = clock();
    int W = 1024; //
    int H = 1024; //512;
    // integrateCos();
    // integrate4D();
    // return 0;

    Vector C(0, 30, 55);
    Scene scene;
    scene.I = 6E9;
    scene.L = Vector(-10, 50, 40);

    Sphere Slum(scene.L, 5, Vector(1., 1., 1.));
    Sphere S4(Vector(-15, 30, -30), 10, Vector(0., 0., 1.));
    Sphere S5(Vector(0, 30, -30), 10, Vector(1., 1., 1.), true);
    Sphere S6(Vector(15, 30, -30), 10, Vector(1., 0., 0.), false, true);
    Sphere S1(Vector(-15, 0, 0), 10, Vector(0., 0., 1.));
    Sphere S2(Vector(0, 0, 0), 10, Vector(1., 1., 1.), true);
    Sphere S3(Vector(15, 0, 0), 10, Vector(1., 0., 0.), false, true);
    Sphere Smurga(Vector(-1050, 0, 0), 970, Vector(138 / 255., 168 / 255., 167 / 255.)); // Vector(0., 0., 1.));
    Sphere Smurdr(Vector(1050, 0, 0), 970, Vector(138 / 255., 168 / 255., 167 / 255.));  // Vector(1., 0., 0.));
    Sphere Smurfa(Vector(0, 0, -1050), 940, Vector(138 / 255., 168 / 255., 167 / 255.)); // Vector(0., 1., 0.));
    Sphere Smurde(Vector(0, 0, 1000), 940, Vector(138 / 255., 168 / 255., 167 / 255.));  // Vector(1., 0., 1.));
    Sphere Ssol(Vector(0, -1000, 0), 990, Vector(1., 1., 1.), false);
    Sphere Splafond(Vector(0, 1000, 0), 990, Vector(1., 1., 1.));
    TriangleMesh m(Vector(0., 1., 1.), false, false);
    TriangleMesh m2(Vector(1., 0., 0.), false, false);
    TriangleMesh m3(Vector(1., 1., 1.), false, false);
    TriangleMesh mplane(Vector(16. / 255, 133. / 255, 49. / 255), false, false);
    TriangleMesh mplanejapan(Vector(1., 0., 0.), false, false);
    TriangleMesh mtank(Vector(0., 0., 1.), false, false);
    TriangleMesh mtank2(Vector(0., 0., 1.), false, false);
    TriangleMesh mtankpanzer(Vector(1., 0., 0.), false, false);
    TriangleMesh mtankpanzer1(Vector(1., 0., 0.), false, false);
    TriangleMesh mgresoldier(Vector(0., 0., 1.), false, false);
    TriangleMesh mgresoldier1(Vector(0., 0., 1.), false, false);
    TriangleMesh mrifsoldier(Vector(0., 0., 1.), false, false);
    TriangleMesh mrifsoldier1(Vector(0., 0., 1.), false, false);
    TriangleMesh mbazsoldier(Vector(0., 0., 1.), false, false);
    TriangleMesh mbazsoldier1(Vector(0., 0., 1.), false, false);
    Sphere SMm(Vector(20, 20, -10), 10, Vector(1., 1., 1.), true);
    Sphere STm(Vector(0, 0, 10), 10, Vector(1., 1., 1.), false, true);
    // m.readOBJ("./objects/chien/13463_Australian_Cattle_Dog_v3.obj");
    // m.zoom(0.3);
    // m.loadTexture("./objects/chien/Australian_Cattle_Dog_dif.jpg");
    // // inversion y et z
    // m.rotate(1, 2);
    // // inversion x et z
    // m.rotate(0, 2);
    // m.translate(1, -10);
    // m.invertNormals();
    // m.rotateNormals(1, 2);
    // m.rotateNormals(0, 2);

    // m2.readOBJ("./objects/dumbell/10499_Dumbells_v1_L3.obj");
    // // O vers la droite 1 vers le haut 2 profondeur
    // m2.translate(1, -5);
    // m2.translate(2, 0);
    // m2.translate(2, -20);
    // m2.loadTexture("./objects/dumbell/10499_Dumbells_v1_diffuse.jpg");

    // m3.readOBJ("./objects/dumbell/10499_Dumbells_v1_L3.obj");
    // m3.loadTexture("./objects/dumbell/10499_Dumbells_v1_diffuse.jpg");
    // m3.translate(0, -10);
    // m3.translate(1, -5);
    // m3.translate(2, 0); // inversion de l'axe

    mgresoldier.readOBJ("./objects/soldiers/grenade_soldier/14073_WWII_Soldier_throwing_grenade_v2_L1.obj");
    mgresoldier.loadTexture("./objects/soldiers/grenade_soldier/14073_WWII_Soldier_throwing_grenade_diff.jpg");
    mgresoldier.zoom(10);
    mgresoldier.rotate(1, 2);
    mgresoldier.rotate(0, 2);
    mgresoldier.translate(1, -10);
    mgresoldier.rotateAroundAxe(1, -30);
    mgresoldier.translate(0, 30);
    mgresoldier.translate(2, -15);

    mgresoldier1.readOBJ("./objects/soldiers/grenade_soldier/14073_WWII_Soldier_throwing_grenade_v2_L1.obj");
    mgresoldier1.loadTexture("./objects/soldiers/grenade_soldier/14073_WWII_Soldier_throwing_grenade_diff.jpg");
    mgresoldier1.zoom(10);
    mgresoldier1.rotate(1, 2);
    mgresoldier1.rotate(0, 2);
    mgresoldier1.translate(0, 5);
    mgresoldier1.translate(1, -10);
    mgresoldier1.rotateAroundAxe(1, -40);
    mgresoldier1.translate(0, 18);
    mgresoldier1.translate(2, 13);

    mrifsoldier.readOBJ("./objects/soldiers/rifle_soldier/14070_WWII_Soldier_with_Rife_v1_L1.obj");
    mrifsoldier.loadTexture("./objects/soldiers/rifle_soldier/14070_WWII_Soldier_with_Rifle_diff.jpg");
    mrifsoldier.zoom(4);
    mrifsoldier.rotate(1, 2);
    mrifsoldier.rotate(0, 2);
    mrifsoldier.translate(1, -10);
    mrifsoldier.rotateAroundAxe(1, -30);
    mrifsoldier.translate(0, 25);
    mrifsoldier.translate(2, -20);

    mrifsoldier1.readOBJ("./objects/soldiers/rifle_soldier/14070_WWII_Soldier_with_Rife_v1_L1.obj");
    mrifsoldier1.loadTexture("./objects/soldiers/rifle_soldier/14070_WWII_Soldier_with_Rifle_diff.jpg");
    mrifsoldier1.zoom(4);
    mrifsoldier1.rotate(1, 2);
    mrifsoldier1.rotate(0, 2);
    mrifsoldier1.translate(1, -10);
    mrifsoldier1.rotateAroundAxe(1, -10);
    mrifsoldier1.translate(0, 15);
    mrifsoldier1.translate(2, 18);

    mbazsoldier.readOBJ("./objects/soldiers/bazooka_soldier/14071_WWII_Soldier_with_Bazooka_v1_L1.obj");
    mbazsoldier.loadTexture("./objects/soldiers/bazooka_soldier/14071_WWII_Soldier_with_Bazooka_diff.jpg");
    mbazsoldier.zoom(4);
    mbazsoldier.rotate(1, 2);
    mbazsoldier.rotate(0, 2);
    mbazsoldier.translate(1, -10);
    mbazsoldier.rotateAroundAxe(1, -30);
    mbazsoldier.translate(0, 20);
    mbazsoldier.translate(2, -18);

    mbazsoldier1.readOBJ("./objects/soldiers/bazooka_soldier/14071_WWII_Soldier_with_Bazooka_v1_L1.obj");
    mbazsoldier1.loadTexture("./objects/soldiers/bazooka_soldier/14071_WWII_Soldier_with_Bazooka_diff.jpg");
    mbazsoldier1.zoom(4);
    mbazsoldier1.rotate(1, 2);
    mbazsoldier1.rotate(0, 2);
    mbazsoldier1.translate(1, -10);
    mbazsoldier1.rotateAroundAxe(1, -10);
    mbazsoldier1.translate(0, 28);
    mbazsoldier1.translate(2, 15);

    mplane.readOBJ("./objects/airplane/10593_Fighter_Jet_SG_v1_iterations-2.obj");
    // mplane.loadTexture("./objects/airplane/10593_Fighter_Jet_SG_v1_diffuse.jpg");
    mplane.zoom(0.8);
    mplane.rotate(1, 2);
    mplane.rotate(0, 2);
    mplane.translate(1, 28); //15
    mplane.translate(0, -15);
    // mplane.translate(2, -10);
    mplane.rotateAroundAxe(1, 30);
    mplane.rotateAroundAxe(0, 15);

    mplanejapan.readOBJ("./objects/airplane_japan/14082_WWII_Plane_Japan_Kawasaki_Ki-61_v1_L2.obj");
    // mplanejapan.loadTexture("./objects/airplane_japan/14082_WWII_Plane_Japan_Kawasaki_Ki-61_diffuse_v1.jpg");
    mplanejapan.zoom(4);
    mplanejapan.rotate(1, 2);
    mplanejapan.rotate(0, 2);
    mplanejapan.rotateAroundAxe(1, -30);
    // mplanejapan.rotateAroundAxe(0, 15);
    mplanejapan.translate(1, 31); //15
    mplanejapan.translate(0, 5);
    mplanejapan.translate(2, 25);

    mtank.readOBJ("./objects/tank/14078_WWII_Tank_Soviet_Union_T-70_v1_l2.obj");
    mtank.loadTexture("./objects/tank/14078_WWII_Tank_Soviet_Union_T-70_hull_diff.jpg");
    mtank.loadTexture("./objects/tank/14078_WWII_Tank_Soviet_Union_T-70_tracks_diff.jpg");
    mtank.loadTexture("./objects/tank/14078_WWII_Tank_Soviet_Union_T-70_turret_diff.jpg");
    mtank.zoom(4);
    // mtank.translate(2, -10);
    mtank.rotate(1, 2);
    mtank.translate(1, -11);
    mtank.rotate(0, 2);
    mtank.translate(2, 0);
    mtank.rotateAroundAxe(1, 70);
    mtank.translate(0, 20);

    mtank2.readOBJ("./objects/tank/14078_WWII_Tank_Soviet_Union_T-70_v1_l2.obj");
    mtank2.loadTexture("./objects/tank/14078_WWII_Tank_Soviet_Union_T-70_hull_diff.jpg");
    mtank2.loadTexture("./objects/tank/14078_WWII_Tank_Soviet_Union_T-70_tracks_diff.jpg");
    mtank2.loadTexture("./objects/tank/14078_WWII_Tank_Soviet_Union_T-70_turret_diff.jpg");
    mtank2.zoom(4);
    mtank2.rotate(1, 2);
    mtank2.rotate(0, 2);
    mtank2.translate(2, 0);
    mtank2.rotateAroundAxe(1, 95);
    mtank2.translate(0, 25);
    mtank2.translate(1, -11);
    mtank2.translate(2, -30);

    mtankpanzer.readOBJ("./objects/tank_panzer/14077_WWII_Tank_Germany_Panzer_III_v1_L2.obj");
    mtankpanzer.loadTexture("./objects/tank_panzer/14077_WWII_Tank_Germany_Panzer_III_hull_diff.jpg");
    mtankpanzer.loadTexture("./objects/tank_panzer/14077_WWII_Tank_Germany_Panzer_III_tracks_diff.jpg");
    mtankpanzer.loadTexture("./objects/tank_panzer/14077_WWII_Tank_Germany_Panzer_III_turret_diff.jpg");
    mtankpanzer.zoom(5);
    mtankpanzer.rotate(1, 2);
    mtankpanzer.rotate(0, 2);
    mtankpanzer.translate(2, 0);
    mtankpanzer.rotateAroundAxe(1, -60);
    mtankpanzer.translate(1, -11);
    mtankpanzer.translate(0, -20);
    // mtankpanzer.translate(2, -20);

    mtankpanzer1.readOBJ("./objects/tank_panzer/14077_WWII_Tank_Germany_Panzer_III_v1_L2.obj");
    mtankpanzer1.loadTexture("./objects/tank_panzer/14077_WWII_Tank_Germany_Panzer_III_hull_diff.jpg");
    mtankpanzer1.loadTexture("./objects/tank_panzer/14077_WWII_Tank_Germany_Panzer_III_tracks_diff.jpg");
    mtankpanzer1.loadTexture("./objects/tank_panzer/14077_WWII_Tank_Germany_Panzer_III_turret_diff.jpg");
    mtankpanzer1.zoom(5);
    mtankpanzer1.rotate(1, 2);
    mtankpanzer1.rotate(0, 2);
    mtankpanzer1.translate(2, 0);
    mtankpanzer1.rotateAroundAxe(1, -90);
    mtankpanzer1.translate(1, -11);
    mtankpanzer1.translate(0, -25);
    mtankpanzer1.translate(2, -30);

    // std::ofstream meanfile;

    // meanfile.open("meanfile.txt");

    // double meanM0 = 1., meanM1 = 1., meanM2 = 1.;

    // for (int i = 0; i < m.vertices.size(); i++)
    // {
    //     meanM0 += m.vertices[i][0];
    //     meanM1 += m.vertices[i][1];
    //     meanM2 += m.vertices[i][2];
    // }
    // meanM0 = meanM0 / m.vertices.size();
    // meanM1 = meanM1 / m.vertices.size();
    // meanM2 = meanM2 / m.vertices.size();

    // meanfile << "chien";
    // meanfile << std::to_string(meanM0) + " ";
    // meanfile << std::to_string(meanM1) + " ";
    // meanfile << std::to_string(meanM2) + "\n";

    // meanfile.close();

    // buildBVH
    // m.buildBVH(m.BVH, 0, m.indices.size());
    // m2.buildBVH(m2.BVH, 0, m2.indices.size());
    // m3.buildBVH(m3.BVH, 0, m3.indices.size());

    mgresoldier.buildBVH(mgresoldier.BVH, 0, mgresoldier.indices.size());
    mgresoldier1.buildBVH(mgresoldier1.BVH, 0, mgresoldier1.indices.size());

    mrifsoldier.buildBVH(mrifsoldier.BVH, 0, mrifsoldier.indices.size());
    mrifsoldier1.buildBVH(mrifsoldier1.BVH, 0, mrifsoldier1.indices.size());

    mbazsoldier.buildBVH(mbazsoldier.BVH, 0, mbazsoldier.indices.size());
    mbazsoldier1.buildBVH(mbazsoldier1.BVH, 0, mbazsoldier1.indices.size());

    mplane.buildBVH(mplane.BVH, 0, mplane.indices.size());
    mplanejapan.buildBVH(mplanejapan.BVH, 0, mplanejapan.indices.size());
    mtank.buildBVH(mtank.BVH, 0, mtank.indices.size());
    mtank2.buildBVH(mtank2.BVH, 0, mtank2.indices.size());
    mtankpanzer.buildBVH(mtankpanzer.BVH, 0, mtankpanzer.indices.size());
    mtankpanzer1.buildBVH(mtankpanzer1.BVH, 0, mtankpanzer1.indices.size());

    scene.objects.push_back(&Slum);
    // scene.objects.push_back(&S4);
    // scene.objects.push_back(&S5);
    // scene.objects.push_back(&S6);
    // scene.objects.push_back(&S1);
    // scene.objects.push_back(&S2);
    // scene.objects.push_back(&S3);

    scene.objects.push_back(&Smurga); //
    scene.objects.push_back(&Smurdr); //
    scene.objects.push_back(&Smurfa); //
    scene.objects.push_back(&Smurde);
    scene.objects.push_back(&Ssol); //
    // scene.objects.push_back(&Splafond);
    // chien
    // scene.objects.push_back(&m);
    // dumbell
    // scene.objects.push_back(&m2);
    // scene.objects.push_back(&m3);

    scene.objects.push_back(&mgresoldier);
    scene.objects.push_back(&mgresoldier1);

    scene.objects.push_back(&mrifsoldier);
    scene.objects.push_back(&mrifsoldier1);

    scene.objects.push_back(&mbazsoldier);
    scene.objects.push_back(&mbazsoldier1);
    //* list of all objects
    scene.objects.push_back(&mplane);
    scene.objects.push_back(&mplanejapan);
    scene.objects.push_back(&mtank);
    scene.objects.push_back(&mtank2);
    scene.objects.push_back(&mtankpanzer);
    scene.objects.push_back(&mtankpanzer1);
    //*/
    // scene.objects.push_back(&SMm);

    // scene.objects.push_back(&STm);

    double fov = 60 * M_PI / 180;

    int nbrays = 1;                          //2;
    double angleVertical = -25 * M_PI / 180, //-30
        angleHorizontal = 0 * M_PI / 180;    //10
    Vector up(0, cos(angleVertical), sin(angleVertical));
    Vector right(cos(angleHorizontal), 0, sin(angleHorizontal));

    double up0 = up[0];
    up[0] = cos(angleHorizontal) * up[0] - sin(angleHorizontal) * up[2];
    up[2] = sin(angleHorizontal) * up0 + cos(angleHorizontal) * up[2];

    Vector viewDirection = cross(up, right);

    std::vector<unsigned char> image(W * H * 3, 0);
#pragma omp parallel for schedule(dynamic, 1)
    for (int i = 0; i < H; i++)
    {
        for (int j = 0; j < W; j++)
        {

            //2
            Vector color(0, 0, 0);
            for (int k = 0; k < nbrays; k++)
            {

                double u1 = uniform(engine), u2 = uniform(engine);
                double x1 = 0.25 * cos(2 * M_PI * u1) * sqrt(-2 * log(u2)),
                       x2 = 0.25 * sin(2 * M_PI * u1) * sqrt(-2 * log(u2));

                double u3 = uniform(engine), u4 = uniform(engine);
                double x3 = 0.01 * cos(2 * M_PI * u3) * sqrt(-2 * log(u4)), // remettre à 1 pour la profondeur de champ 0.01 pour l'enlever
                    x4 = 0.01 * sin(2 * M_PI * u3) * sqrt(-2 * log(u4));

                Vector u(j - W / 2 + x2 + 0.5, i - H / 2 + x1 + 0.5, W / (2. * tan(fov / 2)));
                u = u.get_normalized();
                u = u[0] * right + u[1] * up + u[2] * viewDirection;

                Vector target = C + 55 * u;
                Vector Cprime = C + Vector(x3, x4, 0);
                Vector uprime = (target - Cprime).get_normalized();
                // Ray r(C, u);
                Ray r(Cprime, uprime);

                color += scene.getColor(r, 0, false);
            }

            color = color / nbrays;

            // implementation de la transmission de Fresnel avec tirage aleatoire et moyenne
            // Vector color = Vector(0., 0., 0.);
            // int nbVal = 30;
            // for (int i = 0; i < nbVal; i++)
            // {
            //     color = color + scene.getColor(r, 0);
            // }
            // color = color / nbVal;

            image[((H - i - 1) * W + j) * 3 + 0] = std::min(255., std::pow(color[0], 0.45));
            image[((H - i - 1) * W + j) * 3 + 1] = std::min(255., std::pow(color[1], 0.45));
            image[((H - i - 1) * W + j) * 3 + 2] = std::min(255., std::pow(color[2], 0.45));
        }
    }

    // wwriting the image on a png file
    stbi_write_png("raytracer.png", W, H, 3, &image[0], 0);

    // writing execution time on a txt file
    std::ofstream execution_file;
    execution_file.open("execution_time.txt");
    execution_file << (clock() - ini_time) / CLOCKS_PER_SEC;
    execution_file.close();

    return 0;
};

// run command line
// g++ raytracer.cpp -o raytracer.exe -O3 -fopenmp