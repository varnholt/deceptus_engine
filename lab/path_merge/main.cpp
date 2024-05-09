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
constexpr auto triangulate = false;
}


struct Vertex {
   uint32_t _index_pos = 0;
   uint32_t _index_normal = 0;
   uint32_t _index_texcoord = 0;
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

   auto face_count = 0u;

   std::ifstream obj_stream(filename, std::ios::in);

   if (!obj_stream)
   {
      std::cerr << "unable to open file: " << filename << std::endl;
      return;
   }

   std::string line, token;

   getline(obj_stream, line);

   while (!obj_stream.eof())
   {
     trimString(line);

     if (line.length( ) > 0 && line.at(0) != '#')
     {
         std::istringstream line_stream(line);

         line_stream >> token;

         if (token == "v")
         {
             float x = 0.0f;
             float y = 0.0f;
             float z = 0.0f;

             line_stream >> x >> y >> z;
             points.push_back(QVector3D(x, y, z));
         }
         else if (token == "vt")
         {
             float s = 0.0f;
             float t = 0.0f;

             line_stream >> s >> t;
             uvs.push_back(QVector2D(s, t));

         } else if (token == "vn")
         {
            float x = 0.0f;
            float y = 0.0f;
            float z = 0.0f;

            line_stream >> x >> y >> z;
            normals.push_back(QVector3D(x, y, z));
         }
         else if (token == "f")
         {
            face_count++;

            std::vector<uint32_t> face;

            size_t slash1 = 0;
            size_t slash2 = 0;

            while (line_stream.good())
            {
               std::string vert_string;
               line_stream >> vert_string;

               auto index_pos = 0u;
               auto index_normal = 0u;
               auto index_texcoord = 0u;

               slash1 = vert_string.find("/");

               if (slash1 == std::string::npos)
               {
                  index_pos = static_cast<uint32_t>(atoi(vert_string.c_str()) - 1);
               }
               else
               {
                  slash2 = vert_string.find("/", slash1 + 1);
                  index_pos = static_cast<uint32_t>(atoi( vert_string.substr(0,slash1).c_str() ) - 1);

                  if( slash2 > slash1 + 1 )
                  {
                     index_texcoord = static_cast<uint32_t>(atoi(vert_string.substr(slash1 + 1, slash2).c_str() ) - 1);
                  }

                  index_normal = static_cast<uint32_t>(atoi( vert_string.substr(slash2 + 1,vert_string.length()).c_str() ) - 1);
               }

               Vertex vertex;
               vertex._index_pos = index_pos;
               vertex._index_normal = index_normal;
               vertex._index_texcoord = index_texcoord;

               face.push_back(index_pos);
               vertices.push_back(vertex);
            }

            // obj: f pos/tex/norm  pos/tex/norm  pos/tex/norm

            // If number of edges in face is greater than 3,
            // decompose into triangles as a triangle fan.

            std::vector<uint32_t> face_indices;
            if (face.size() > 3 && triangulate)
            {
               auto v0 = face[0];
               auto v1 = face[1];
               auto v2 = face[2];

               Vertex vt0 = vertices[0];
               Vertex vt1 = vertices[1];
               Vertex vt2 = vertices[2];

               face_indices.push_back(v0);
               face_indices.push_back(v1);
               face_indices.push_back(v2);

               vertices.push_back(vt0);
               vertices.push_back(vt1);
               vertices.push_back(vt2);

               for (auto i = 3u; i < face.size(); i++)
               {
                  v1 = v2;
                  v2 = face[i];

                  vt1 = vt2;
                  vt2 = vertices[i];

                  face_indices.push_back(v0);
                  face_indices.push_back(v1);
                  face_indices.push_back(v2);

                  vertices.push_back(vt0);
                  vertices.push_back(vt1);
                  vertices.push_back(vt2);
               }
            }
            else
            {
               for (auto i = 0u; i < face.size(); i++)
               {
                  face_indices.push_back(face[i]);
                  vertices.push_back(vertices[i]);
               }
            }

            faces.push_back(face_indices);
         }
      }

      getline(obj_stream, line);
   }

   obj_stream.close();

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
         const auto face_index = f[i];
         const auto& point = points[face_index];
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
   auto simplified_polys = simplified.toSubpathPolygons(transform);

   std::vector<std::vector<uint32_t>> simplified_faces;
   std::vector<QVector3D> simplified_points;

   for (const auto& poly : simplified_polys)
   {
      std::vector<uint32_t> face;
      for (const auto& point : poly)
      {
         const auto& it = std::find_if(simplified_points.begin(), simplified_points.end(), [point](const QVector3D& other){
            return (
                  fabs(point.x() - other.x()) < 0.001f
               && fabs(point.y() - other.y()) < 0.001f
            );
         });

         uint32_t vertex_index = 0;
         QVector3D vec(point.x(), point.y(), 0.0f);
         if (it == simplified_points.end())
         {
            vertex_index = simplified_points.size();
            simplified_points.push_back(vec);
         }
         else
         {
            vertex_index = it - simplified_points.begin();
         }

         face.push_back(vertex_index + 1);
      }

      simplified_faces.push_back(face);
   }

   writeObj(out, simplified_points, simplified_faces);

   std::cout
      << "optimised mesh written to '"
      << out
      << "', points: "
      << points.size()
      << " -> "
      << simplified_points.size()
      << ", faces: "
      << path.elementCount()
      << " -> "
      << simplified.elementCount()
      << ", factor: "
      << simplified_points.size() / static_cast<float>(points.size())
      << std::endl;

   return 0;
}



