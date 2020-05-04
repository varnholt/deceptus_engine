#include <deque>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <sstream>

#include <math.h>

#include <QPainterPath>
#include <QPolygonF>
#include <QTransform>
#include <QVector2D>
#include <QVector3D>


namespace {
static const auto triangulate = false;
}


struct Vertex {
   uint32_t pIndex = 0;
   uint32_t nIndex = 0;
   uint32_t tcIndex = 0;
};


void readObj(
   const std::string& filename,
   std::vector<QVector3D>& points,
   std::vector<std::vector<uint32_t>>& faces
)
{
   std::vector<Vertex> vertices;
   std::vector<QVector3D> normals;
   std::vector<QVector2D> uvs;

   auto trimString = [](std::string& str) {
      const char* whiteSpace = " \t\n\r";
      size_t location = str.find_first_not_of(whiteSpace);
      str.erase(0, location);
      location = str.find_last_not_of(whiteSpace);
      str.erase(location + 1);
   };

   auto faceCount = 0u;

   std::ifstream objStream(filename, std::ios::in);

   if (!objStream)
   {
      std::cerr << "unable to open file: " << filename << std::endl;
      return;
   }

   std::string line, token;

   getline(objStream, line);

   while (!objStream.eof())
   {
     trimString(line);

     if (line.length( ) > 0 && line.at(0) != '#')
     {
         std::istringstream lineStream(line);

         lineStream >> token;

         if (token == "v")
         {
             float x = 0.0f;
             float y = 0.0f;
             float z = 0.0f;

             lineStream >> x >> y >> z;
             points.push_back(QVector3D(x, y, z));
         }
         else if (token == "vt")
         {
             float s = 0.0f;
             float t = 0.0f;

             lineStream >> s >> t;
             uvs.push_back(QVector2D(s, t));

         } else if (token == "vn")
         {
            float x = 0.0f;
            float y = 0.0f;
            float z = 0.0f;

            lineStream >> x >> y >> z;
            normals.push_back(QVector3D(x, y, z));
         }
         else if (token == "f")
         {
            faceCount++;

            std::vector<uint32_t> face;

            size_t slash1 = 0;
            size_t slash2 = 0;

            while (lineStream.good())
            {
               std::string vertString;
               lineStream >> vertString;

               auto pIndex = 0u;
               auto nIndex = 0u;
               auto tcIndex = 0u;

               slash1 = vertString.find("/");

               if (slash1 == std::string::npos)
               {
                  pIndex = static_cast<uint32_t>(atoi(vertString.c_str()) - 1);
               }
               else
               {
                  slash2 = vertString.find("/", slash1 + 1);
                  pIndex = static_cast<uint32_t>(atoi( vertString.substr(0,slash1).c_str() ) - 1);

                  if( slash2 > slash1 + 1 )
                  {
                     tcIndex = static_cast<uint32_t>(atoi(vertString.substr(slash1 + 1, slash2).c_str() ) - 1);
                  }

                  nIndex = static_cast<uint32_t>(atoi( vertString.substr(slash2 + 1,vertString.length()).c_str() ) - 1);
               }

               Vertex vertex;
               vertex.pIndex = pIndex;
               vertex.nIndex = nIndex;
               vertex.tcIndex = tcIndex;

               face.push_back(pIndex);
               vertices.push_back(vertex);
            }

            // obj: f pos/tex/norm  pos/tex/norm  pos/tex/norm

            // If number of edges in face is greater than 3,
            // decompose into triangles as a triangle fan.

            std::vector<uint32_t> faceIndices;
            if (face.size() > 3 && triangulate)
            {
               auto v0 = face[0];
               auto v1 = face[1];
               auto v2 = face[2];

               Vertex vt0 = vertices[0];
               Vertex vt1 = vertices[1];
               Vertex vt2 = vertices[2];

               faceIndices.push_back(v0);
               faceIndices.push_back(v1);
               faceIndices.push_back(v2);

               vertices.push_back(vt0);
               vertices.push_back(vt1);
               vertices.push_back(vt2);

               for (auto i = 3u; i < face.size(); i++)
               {
                  v1 = v2;
                  v2 = face[i];

                  vt1 = vt2;
                  vt2 = vertices[i];

                  faceIndices.push_back(v0);
                  faceIndices.push_back(v1);
                  faceIndices.push_back(v2);

                  vertices.push_back(vt0);
                  vertices.push_back(vt1);
                  vertices.push_back(vt2);
               }
            }
            else
            {
               for (auto i = 0u; i < face.size(); i++)
               {
                  faceIndices.push_back(face[i]);
                  vertices.push_back(vertices[i]);
               }
            }

            faces.push_back(faceIndices);
         }
      }

      getline(objStream, line);
   }

   objStream.close();

   // std::cout << "Loaded mesh from: " << filename << std::endl;
   //
   // std::cout << " " << points.size()     << " points"      << std::endl;
   // std::cout << " " << faceCount         << " faces"       << std::endl;
   // std::cout << " " << faces.size() / 3  << " triangles."  << std::endl;
   // std::cout << " " << normals.size()    << " normals"     << std::endl;
   // std::cout << " " << uvs.size()        << " uvs"         << std::endl;
}


void writeObj(
   const std::string& filename,
   const std::vector<QVector3D>& vertices,
   const std::vector<std::vector<uint32_t>>& faces
)
{
   std::ofstream out(filename);

   out.setf(std::ios::fixed);
   for (const auto& v : vertices)
   {
      out << std::setprecision(3) << "v " << v.x() << " " << v.y() << " " << 0.0f << std::endl;
   }

   out << std::endl;

   for (const auto& face : faces)
   {
      out << "f ";
      for (const auto p : face)
      {
         out << p << " ";
      }
      out << std::endl;
   }

   out.close();
}


int main(int32_t argc, char** argv)
{
   if (argc != 3)
   {
      std::cout << "usage: " << argv[0] << " input_file.obj output_file.obj" << std::endl;
      exit(0);
   }

   // auto in = "C:\\git\\build\\layer_level.obj";
   // auto out = "C:\\git\\build\\layer_level_out.obj";
   auto in = argv[1];
   auto out = argv[2];

   std::vector<QVector3D> points;
   std::vector<std::vector<uint32_t>> faces;

   readObj(in, points, faces);

   std::vector<QPolygonF> polys;
   for (auto& f : faces)
   {
      QPolygonF poly;

      for (auto i = 0u; i < f.size(); i++)
      {
         const auto faceIndex = f[i];
         const auto& point = points[faceIndex];
         poly.append({point.x(), point.y()});
      }

      polys.push_back(poly);
   }

   QPainterPath path;
   for (auto& p : polys)
   {
      path.addPolygon(p);
   }

   QPainterPath simplified = path.simplified();

   QTransform transform;
   auto simplifiedPolys = simplified.toSubpathPolygons(transform);


   std::vector<std::vector<uint32_t>> simplifiedFaces;
   std::vector<QVector3D> simplifiedPoints;

   for (auto& poly : simplifiedPolys)
   {
      std::vector<uint32_t> face;
      for (auto& point : poly)
      {
         const auto& it = std::find_if(simplifiedPoints.begin(), simplifiedPoints.end(), [point](const QVector3D& other){
            return (
                  fabs(point.x() - other.x()) < 0.001f
               && fabs(point.y() - other.y()) < 0.001f
            );
         });

         uint32_t vertexIndex = 0;
         QVector3D vec(point.x(), point.y(), 0.0f);
         if (it == simplifiedPoints.end())
         {
            vertexIndex = simplifiedPoints.size();
            simplifiedPoints.push_back(vec);
         }
         else
         {
            vertexIndex = it - simplifiedPoints.begin();
         }

         face.push_back(vertexIndex + 1);
      }

      simplifiedFaces.push_back(face);
   }

   writeObj(out, simplifiedPoints, simplifiedFaces);

   std::cout
      << "optimised mesh written to '"
      << out
      << "', points: "
      << points.size()
      << " -> "
      << simplifiedPoints.size()
      << ", faces: "
      << path.elementCount()
      << " -> "
      << simplified.elementCount()
      << ", factor: "
      << simplifiedPoints.size() / static_cast<float>(points.size())
      << std::endl;

   return 0;
}



