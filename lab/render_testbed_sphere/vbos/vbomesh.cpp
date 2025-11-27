#include "vbomesh.h"

#include "opengl/glutils.h"

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>


VBOMesh::VBOMesh(const char * fileName, float scale, bool center, bool loadTc, bool genTangents)
 : _scale(scale),
   _recenter_mesh(center),
   _load_texture(loadTc),
   _generate_tangents(genTangents)
{
   loadObj(fileName);
}


void VBOMesh::render() const
{
    glBindVertexArray(_vao_handle);
    glDrawElements(GL_TRIANGLES, 3 * _faces, GL_UNSIGNED_INT, ((GLubyte *)NULL + (0)));
}


void VBOMesh::loadObj(const char * fileName)
{
    std::vector <glm::vec3> points;
    std::vector <glm::vec3> normals;
    std::vector <glm::vec2> texCoords;
    std::vector <GLuint> faces;
    std::vector<Vertex> vertices;

    int nFaces = 0;

    std::ifstream objStream(fileName, std::ios::in);

    if (!objStream) {
        std::cerr << "Unable to open OBJ file: " << fileName << std::endl;
        exit(1);
    }

    std::string line, token;
    std::vector<int> face;
    std::vector<Vertex> faceVertices;

    getline(objStream, line);
    while(!objStream.eof())
    {
        trimString(line);
        if (line.length() > 0 && line.at(0) != '#')
        {
            std::istringstream lineStream(line);

            lineStream >> token;

            if (token == "v")
            {
                float x, y, z;
                lineStream >> x >> y >> z;
                points.push_back(glm::vec3(x,y,z) * _scale);
            }
            else if (token == "vt" && _load_texture)
            {
                // Process texture coordinate
                float s,t;
                lineStream >> s >> t;
                texCoords.push_back(glm::vec2(s,t));

            } else if (token == "vn")
            {
                float x, y, z;
                lineStream >> x >> y >> z;
                normals.push_back(glm::vec3(x,y,z));
            }
            else if (token == "f")
            {
                nFaces++;

                // Process face
                face.clear();
                faceVertices.clear();

                size_t slash1, slash2;

                //int point, texCoord, normal;
                while(lineStream.good())
                {
                    std::string vertString;
                    lineStream >> vertString;
                    int pIndex = -1, nIndex = -1 , tcIndex = -1;

                    slash1 = vertString.find("/");

                    if (slash1 == std::string::npos)
                    {
                        pIndex = atoi(vertString.c_str()) - 1;
                    }
                    else
                    {
                        slash2 = vertString.find("/", slash1 + 1);
                        pIndex = atoi(vertString.substr(0,slash1).c_str())
                                        - 1;
                        if (slash2 > slash1 + 1)
                        {
                          tcIndex =
                                  atoi(vertString.substr(slash1 + 1, slash2).c_str())
                                  - 1;
                        }
                        nIndex =
                                atoi(vertString.substr(slash2 + 1,vertString.length()).c_str())
                                - 1;
                    }

                    if (pIndex == -1)
                    {
                        printf("Missing point index!!!");
                    }
                    else
                    {
                       Vertex vertex;
                       vertex.pIndex = pIndex;
                       vertex.nIndex = nIndex;
                       vertex.tcIndex = tcIndex;

                       face.push_back(pIndex);
                       faceVertices.push_back(vertex);
                    }
                }

                // obj: f pos/tex/norm  pos/tex/norm  pos/tex/norm

                // If number of edges in face is greater than 3,
                // decompose into triangles as a triangle fan.
                if (face.size() > 3)
                {
                    int v0 = face[0];
                    int v1 = face[1];
                    int v2 = face[2];

                    Vertex vt0 = faceVertices[0];
                    Vertex vt1 = faceVertices[1];
                    Vertex vt2 = faceVertices[2];

                    // First face
                    faces.push_back(v0);
                    faces.push_back(v1);
                    faces.push_back(v2);

                    vertices.push_back(vt0);
                    vertices.push_back(vt1);
                    vertices.push_back(vt2);

                    for (GLuint i = 3; i < face.size(); i++)
                    {
                        v1 = v2;
                        v2 = face[i];

                        vt1 = vt2;
                        vt2 = faceVertices[i];

                        faces.push_back(v0);
                        faces.push_back(v1);
                        faces.push_back(v2);

                        vertices.push_back(vt0);
                        vertices.push_back(vt1);
                        vertices.push_back(vt2);
                    }
                }
                else
                {
                    faces.push_back(face[0]);
                    faces.push_back(face[1]);
                    faces.push_back(face[2]);

                    vertices.push_back(faceVertices[0]);
                    vertices.push_back(faceVertices[1]);
                    vertices.push_back(faceVertices[2]);

                }
            }
        }
        getline(objStream, line);
    }

    objStream.close();

//    normals.clear();
//    if (normals.size() == 0) {
//        generateAveragedNormals(points,normals,faces);
//    }

    std::vector<glm::vec4> tangents;
    if (_generate_tangents && texCoords.size() > 0) {
        generateTangents(points,normals,faces,texCoords,tangents);
    }

    if (_recenter_mesh) {
        center(points);
    }

    // storeVBO(points, normals, texCoords, tangents, faces);
    storeVBO2(points, normals, texCoords, tangents, faces, vertices);

    std::cout << "Loaded mesh from: " << fileName << std::endl;
    std::cout << " " << points.size() << " points" << std::endl;
    std::cout << " " << nFaces << " faces" << std::endl;
    std::cout << " " << faces.size() / 3 << " triangles." << std::endl;
    std::cout << " " << normals.size() << " normals" << std::endl;
    std::cout << " " << tangents.size() << " tangents " << std::endl;
    std::cout << " " << texCoords.size() << " texture coordinates." << std::endl;
}


void VBOMesh::center(std::vector<glm::vec3>& points)
{
    if (points.size() < 1)
    {
       return;
    }

    glm::vec3 maxPoint = points[0];
    glm::vec3 minPoint = points[0];

    // Find the AABB
   for (GLuint i = 0; i < points.size(); ++i)
   {
      glm::vec3 & point = points[i];

      if (point.x > maxPoint.x) maxPoint.x = point.x;
      if (point.y > maxPoint.y) maxPoint.y = point.y;
      if (point.z > maxPoint.z) maxPoint.z = point.z;
      if (point.x < minPoint.x) minPoint.x = point.x;
      if (point.y < minPoint.y) minPoint.y = point.y;
      if (point.z < minPoint.z) minPoint.z = point.z;
   }

   // Center of the AABB
   glm::vec3 center =
         glm::vec3((maxPoint.x + minPoint.x) / 2.0f,
         (maxPoint.y + minPoint.y) / 2.0f,
         (maxPoint.z + minPoint.z) / 2.0f
      );

   // Translate center of the AABB to the origin
   for (GLuint i = 0; i < points.size(); ++i)
   {
      glm::vec3 & point = points[i];
      point = point - center;
   }
}


void VBOMesh::generateAveragedNormals(
   const std::vector<glm::vec3>& points,
   std::vector<glm::vec3>& normals,
   const std::vector<GLuint>& faces)
{
   for (GLuint i = 0; i < points.size(); i++)
   {
      normals.push_back(glm::vec3(0.0f));
   }

   for (GLuint i = 0; i < faces.size(); i += 3)
   {
      const glm::vec3 & p1 = points[faces[i]];
      const glm::vec3 & p2 = points[faces[i + 1]];
      const glm::vec3 & p3 = points[faces[i + 2]];

      glm::vec3 a = p2 - p1;
      glm::vec3 b = p3 - p1;
      glm::vec3 n = glm::normalize(glm::cross(a,b));

      normals[faces[i]] += n;
      normals[faces[i + 1]] += n;
      normals[faces[i + 2]] += n;
   }

   for (GLuint i = 0; i < normals.size(); i++) {
      normals[i] = glm::normalize(normals[i]);
   }
}


void VBOMesh::generateTangents(
   const std::vector<glm::vec3>& points,
   const std::vector<glm::vec3>& normals,
   const std::vector<GLuint>& faces,
   const std::vector<glm::vec2>& texCoords,
   std::vector<glm::vec4>& tangents)
{
   std::vector<glm::vec3> tan1Accum;
   std::vector<glm::vec3> tan2Accum;

   for (GLuint i = 0; i < points.size(); i++)
   {
      tan1Accum.push_back(glm::vec3(0.0f));
      tan2Accum.push_back(glm::vec3(0.0f));
      tangents.push_back(glm::vec4(0.0f));
   }

   // Compute the tangent std::vector
   for (GLuint i = 0; i < faces.size(); i += 3)
   {
      const glm::vec3 &p1 = points[faces[i]];
      const glm::vec3 &p2 = points[faces[i+1]];
      const glm::vec3 &p3 = points[faces[i+2]];

      const glm::vec2 &tc1 = texCoords[faces[i]];
      const glm::vec2 &tc2 = texCoords[faces[i+1]];
      const glm::vec2 &tc3 = texCoords[faces[i+2]];

      glm::vec3 q1 = p2 - p1;
      glm::vec3 q2 = p3 - p1;
      float s1 = tc2.x - tc1.x, s2 = tc3.x - tc1.x;
      float t1 = tc2.y - tc1.y, t2 = tc3.y - tc1.y;
      float r = 1.0f / (s1 * t2 - s2 * t1);

      glm::vec3 tan1(
         (t2*q1.x - t1*q2.x) * r,
         (t2*q1.y - t1*q2.y) * r,
         (t2*q1.z - t1*q2.z) * r
      );

      glm::vec3 tan2(
         (s1*q2.x - s2*q1.x) * r,
         (s1*q2.y - s2*q1.y) * r,
         (s1*q2.z - s2*q1.z) * r
      );

      tan1Accum[faces[i]] += tan1;
      tan1Accum[faces[i+1]] += tan1;
      tan1Accum[faces[i+2]] += tan1;
      tan2Accum[faces[i]] += tan2;
      tan2Accum[faces[i+1]] += tan2;
      tan2Accum[faces[i+2]] += tan2;
   }

   for (GLuint i = 0; i < points.size(); ++i)
   {
      const glm::vec3 &n = normals[i];
      glm::vec3 &t1 = tan1Accum[i];
      glm::vec3 &t2 = tan2Accum[i];

      // Gram-Schmidt orthogonalize
      tangents[i] = glm::vec4(glm::normalize(t1 - (glm::dot(n,t1) * n)), 0.0f);
      // Store handedness in w
      tangents[i].w = (glm::dot(glm::cross(n,t1), t2) < 0.0f) ? -1.0f : 1.0f;
   }

   tan1Accum.clear();
   tan2Accum.clear();
}


void VBOMesh::storeVBO2(
   const std::vector<glm::vec3>& positions,
   const std::vector<glm::vec3>& normals,
   const std::vector<glm::vec2>&texCoords,
   const std::vector<glm::vec4>&tangents,
   const std::vector<GLuint>&elements,
   const std::vector<Vertex>& vertices
)
{
   GLuint nVerts  = GLuint(vertices.size());
   _faces = GLuint(elements.size() / 3);

   float* v = new float[3 * nVerts];
   float* n = new float[3 * nVerts];
   float* tc = NULL;
   float* tang = NULL;

   if (texCoords.size() > 0)
   {
      tc = new float[ 2 * nVerts];

      if (tangents.size() > 0)
      {
         tang = new float[4*nVerts];
      }
   }

   unsigned int *el = new unsigned int[nVerts];

   int idx = 0, tcIdx = 0, tangIdx = 0;
   for (GLuint i = 0; i < vertices.size(); ++i)
   {
      v[idx]   = positions[vertices.at(i).pIndex].x;
      v[idx+1] = positions[vertices.at(i).pIndex].y;
      v[idx+2] = positions[vertices.at(i).pIndex].z;

      n[idx]   = normals[vertices.at(i).nIndex].x;
      n[idx+1] = normals[vertices.at(i).nIndex].y;
      n[idx+2] = normals[vertices.at(i).nIndex].z;

      idx += 3;

      if (tc != NULL)
      {
         tc[tcIdx]   = texCoords[vertices.at(i).tcIndex].x;
         tc[tcIdx+1] = texCoords[vertices.at(i).tcIndex].y;
         tcIdx += 2;
      }

      if (tang != NULL)
      {
         tang[tangIdx] = tangents[i].x;
         tang[tangIdx + 1] = tangents[i].y;
         tang[tangIdx + 2] = tangents[i].z;
         tang[tangIdx + 3] = tangents[i].w;
         tangIdx += 4;
      }
   }

   for (unsigned int i = 0; i < vertices.size(); ++i)
   {
     el[i] = i;
   }

   glGenVertexArrays(1, &_vao_handle);
   glBindVertexArray(_vao_handle);

   int nBuffers = 3;

   if (tc != NULL)
   {
      nBuffers++;
   }

   if (tang != NULL)
   {
      nBuffers++;
   }

   GLuint elementBuffer = nBuffers - 1;

   GLuint handle[5];
   GLuint bufIdx = 0;
   glGenBuffers(nBuffers, handle);

   glBindBuffer(GL_ARRAY_BUFFER, handle[bufIdx++]);
   glBufferData(GL_ARRAY_BUFFER, (3 * nVerts) * sizeof(float), v, GL_STATIC_DRAW);
   glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, ((GLubyte *)NULL + (0)));
   glEnableVertexAttribArray(0);  // Vertex position

   glBindBuffer(GL_ARRAY_BUFFER, handle[bufIdx++]);
   glBufferData(GL_ARRAY_BUFFER, (3 * nVerts) * sizeof(float), n, GL_STATIC_DRAW);
   glVertexAttribPointer((GLuint)1, 3, GL_FLOAT, GL_FALSE, 0, ((GLubyte *)NULL + (0)));
   glEnableVertexAttribArray(1);  // Vertex normal

   if (tc != NULL)
   {
      glBindBuffer(GL_ARRAY_BUFFER, handle[bufIdx++]);
      glBufferData(GL_ARRAY_BUFFER, (2 * nVerts) * sizeof(float), tc, GL_STATIC_DRAW);
      glVertexAttribPointer((GLuint)2, 2, GL_FLOAT, GL_FALSE, 0, ((GLubyte *)NULL + (0)));
      glEnableVertexAttribArray(2);  // Texture coords
   }

   if (tang != NULL)
   {
      glBindBuffer(GL_ARRAY_BUFFER, handle[bufIdx++]);
      glBufferData(GL_ARRAY_BUFFER, (4 * nVerts) * sizeof(float), tang, GL_STATIC_DRAW);
      glVertexAttribPointer((GLuint)3, 4, GL_FLOAT, GL_FALSE, 0, ((GLubyte *)NULL + (0)));
      glEnableVertexAttribArray(3);  // Tangent std::vector
   }

   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, handle[elementBuffer]);
   glBufferData(GL_ELEMENT_ARRAY_BUFFER, 3 * _faces * sizeof(unsigned int), el, GL_STATIC_DRAW);

   glBindVertexArray(0);

   // Clean up
   delete[] v;
   delete[] n;

   if (tc != NULL)
   {
      delete[] tc;
   }

   if (tang != NULL)
   {
      delete[] tang;
   }

   delete[] el;
}


void VBOMesh::trimString(std::string & str)
{
   const char * whiteSpace = " \t\n\r";
   size_t location;
   location = str.find_first_not_of(whiteSpace);
   str.erase(0,location);
   location = str.find_last_not_of(whiteSpace);
   str.erase(location + 1);
}

