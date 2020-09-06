#ifndef WAVEFRONTOBJ_H
#define WAVEFRONTOBJ_H 

#include <vector>
#include <stdlib.h>
#include <math.h>

#include "Vector.h"
using std::vector;

//==============================================================================
class Vertex {
//==============================================================================
public:
	Vector pos;
	Vector normal;
	int count;

	Vertex(float xval = 0, float yval = 0, float zval = 0) {
		pos.x = xval;
		pos.y = yval;
		pos.z = zval;

		count = 0;
	}
};

//==============================================================================
class TexCoord {
//==============================================================================
public:
	float u, v;

	TexCoord(float u, float v) {
		setCoordinates(u, v);
	}
	void setCoordinates(float uval, float vval) {
		u = uval;
		v = vval;
	}
};

//==============================================================================
class Face {
//==============================================================================
public:
	int vCount;
	int vIndexStart;
	Vector normal;

	Face( int vIndexStart = -1 ) : vIndexStart(vIndexStart), vCount(0) {}
};


//==============================================================================
class WaveFrontOBJ  {
//==============================================================================
public:
	vector<Vertex> verts;
	vector<Vector> normals;
	vector<TexCoord> texCoords;
	vector<Vector> faceNormals;
	vector<Face> faces;
	vector<int> vIndex;
	vector<int> nIndex;
	vector<int> tIndex;

	bool isFlat;
	GLuint mode;

	Vertex bbmin, bbmax; // bounding box

	WaveFrontOBJ(char *filename);
	void Draw();
	void Draw_FN();
	void Draw_VN();

protected:
	Vector faceNormal(Vertex& v0, Vertex& v1, Vertex& v2);
	void vertexNormal();
	void computeBoundingBox();
};


#endif // WAVEFRONTOBJ_H