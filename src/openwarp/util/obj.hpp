#pragma once

#include "../openwarp.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "lib/tiny_obj_loader.h"
#include "lib/stb_image.h"

namespace Openwarp {

	// Struct which is used for vertex attributes.
	// Interleaves position/uv data inside one VBO.
	struct vertex_t {
		GLfloat position[3];
		GLfloat uv[2];
	};

	// Struct for drawable debug objects (scenery, headset visualization, etc)
	// Performs its own GL calls. Not indexed.
	struct object_t {
		GLuint vbo_handle;
		GLuint num_triangles;
		GLuint texture;
		bool has_texture;

		void Draw() {
			glBindBuffer(GL_ARRAY_BUFFER, vbo_handle);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (void*)offsetof(vertex_t, position));
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (void*)offsetof(vertex_t, uv));

			if(has_texture){
				glBindTexture(GL_TEXTURE_2D, texture);
			}

			glDrawArrays(GL_TRIANGLES, 0, num_triangles * 3);

			if(has_texture){
				glBindTexture(GL_TEXTURE_2D, 0);
			}
		}
	};

	// Represents a scene obtained from a single OBJ file.
	// Multiple objects can reside within the OBJ file; each
	// object can have a single diffuse texture. Multi-material
	// objects not supported.
    class ObjScene {
		public:

		// Default constructor for intialization.
		// If the full constructor is not called,
		// Draw() will never do anything.
		ObjScene();

		// obj_dir is the containing directory that should contain
		// the OBJ file, along with any material files and textures.
		//
		// obj_filename is the actual .obj file to be loaded.
		ObjScene(std::string obj_dir, std::string obj_filename);

		void Draw();

		bool successfully_loaded_model = false;
		bool successfully_loaded_texture = false;

		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		
		std::map<std::string, GLuint> textures;

		std::vector<object_t> objects;
	};
}