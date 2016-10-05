#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <string.h>

typedef struct {
  int kind; // 0 = camera, 1 = sphere, 2 = plane
  double color[3];
  union {
    struct {
      double width;
      double height;
    } camera;
    struct {
      double position[3];
      double radius;
    } sphere;
    struct {
      double position[3];
      double normal[3];
    } plane;
  };
} Object;

// creating pixel structure
typedef struct Pixel{
	unsigned char red;
	unsigned char green;
	unsigned char blue;
} Pixel;

Object* object_array[128];
int obj = 0;
int line = 1;

// next_c() wraps the getc() function and provides error checking and line
// number maintenance
int next_c(FILE* json) {
  int c = fgetc(json);
#ifdef DEBUG
  printf("next_c: '%c'\n", c);
#endif
  if (c == '\n') {
    line += 1;
  }
  if (c == EOF) {
    fprintf(stderr, "Error: Unexpected end of file on line number %d.\n", line);
    exit(1);
  }
  return c;
}


// expect_c() checks that the next character is d.  If it is not it emits
// an error.
void expect_c(FILE* json, int d) {
  int c = next_c(json);
  if (c == d) return;
  fprintf(stderr, "Error: Expected '%c' on line %d.\n", d, line);
  exit(1);    
}


// skip_ws() skips white space in the file.
void skip_ws(FILE* json) {
  int c = next_c(json);
  while (isspace(c)) {
    c = next_c(json);
  }
  ungetc(c, json);
}


// next_string() gets the next string from the file handle and emits an error
// if a string can not be obtained.
char* next_string(FILE* json) {
  char buffer[129];
  int c = next_c(json);
  if (c != '"') {
    fprintf(stderr, "Error: Expected string on line %d.\n", line);
    exit(1);
  }  
  c = next_c(json);
  int i = 0;
  while (c != '"') {
    if (i >= 128) {
      fprintf(stderr, "Error: Strings longer than 128 characters in length are not supported.\n");
      exit(1);      
    }
    if (c == '\\') {
      fprintf(stderr, "Error: Strings with escape codes are not supported.\n");
      exit(1);      
    }
    if (c < 32 || c > 126) {
      fprintf(stderr, "Error: Strings may contain only ascii characters.\n");
      exit(1);
    }
    buffer[i] = c;
    i += 1;
    c = next_c(json);
  }
  buffer[i] = 0;
  return strdup(buffer);
}

double next_number(FILE* json) {
  double value;
  fscanf(json, "%lf", &value);
  // Error check this..
  return value;
}

double* next_vector(FILE* json) {
  double* v = malloc(3*sizeof(double));
  expect_c(json, '[');
  skip_ws(json);
  v[0] = next_number(json);
  skip_ws(json);
  expect_c(json, ',');
  skip_ws(json);
  v[1] = next_number(json);
  skip_ws(json);
  expect_c(json, ',');
  skip_ws(json);
  v[2] = next_number(json);
  skip_ws(json);
  expect_c(json, ']');
  return v;
}


void read_scene(char* filename) {
  int c;
  FILE* json = fopen(filename, "r");

  if (json == NULL) {
    fprintf(stderr, "Error: Could not open file \"%s\"\n", filename);
    exit(1);
  }
  
  skip_ws(json);
  
  // Find the beginning of the list
  expect_c(json, '[');

  skip_ws(json);

  // Find the objects

  while (1) {
    c = fgetc(json);
    if (c == ']') {
      fprintf(stderr, "Error: This is the worst scene file EVER.\n");
      fclose(json);
      return;
    }
    if (c == '{') {
      skip_ws(json);
    
      // Parse the object
      char* key = next_string(json);
      if (strcmp(key, "type") != 0) {
		fprintf(stderr, "Error: Expected \"type\" key on line number %d.\n", line);
		exit(1);
      }

      skip_ws(json);

      expect_c(json, ':');

      skip_ws(json);

      char* value = next_string(json);
  	  object_array[obj] = malloc(sizeof(Object));
	  Object new;
      if (strcmp(value, "camera") == 0) {
		(*object_array[obj]).kind = 0;
		printf("Found camera\n");
      } else if (strcmp(value, "sphere") == 0) {
		(*object_array[obj]).kind = 1;
		printf("Found sphere\n");
      } else if (strcmp(value, "plane") == 0) {
		(*object_array[obj]).kind = 2;
		printf("Found plane\n");
      } else {
		fprintf(stderr, "Error: Unknown type, \"%s\", on line number %d.\n", value, line);
		exit(1);
      }

      skip_ws(json);

      while (1) {
		c = next_c(json);
		if (c == '}') {
	  	// stop parsing this object
	  		//object_array[obj] = &new;
			obj++;
	  		break;
	  } else if (c == ',') {
	  	// read another field
		  skip_ws(json);
		  char* key = next_string(json);
		  skip_ws(json);
		  expect_c(json, ':');
		  skip_ws(json);
		// double options///////////////////////////////////////////
	  	if ((strcmp(key, "width") == 0) ||
	      (strcmp(key, "height") == 0) ||
	      (strcmp(key, "radius") == 0)) {
	    	double value = next_number(json);
			if(strcmp(key, "width") == 0){
				if((*object_array[obj]).kind == 0) (*object_array[obj]).camera.width = value;
			}
			else if(strcmp(key, "height") == 0){
				if((*object_array[obj]).kind == 0) (*object_array[obj]).camera.height = value;
			}
			else if(strcmp(key, "radius") == 0){
				(*object_array[obj]).sphere.radius = value;
			}
		///////////////////////////////////////////////////////////
		// vector options//////////////////////////////////////////
	  	} else if ((strcmp(key, "color") == 0) ||
		     (strcmp(key, "position") == 0) ||
		     (strcmp(key, "normal") == 0)) {
	    	double* value = next_vector(json);
			if(strcmp(key, "color") == 0){
				(*object_array[obj]).color[0] = value[0];
				(*object_array[obj]).color[1] = value[1];
				(*object_array[obj]).color[2] = value[2];
			}
			else if(strcmp(key, "position") == 0){
				if((*object_array[obj]).kind == 1){
					(*object_array[obj]).sphere.position[0] = value[0];
					(*object_array[obj]).sphere.position[1] = value[1];
					(*object_array[obj]).sphere.position[2] = value[2];
				}
				else if((*object_array[obj]).kind == 2){
					(*object_array[obj]).plane.position[0] = value[0];
					(*object_array[obj]).plane.position[1] = value[1];
					(*object_array[obj]).plane.position[2] = value[2];
				}
			}
			else if(strcmp(key, "normal") == 0){
				(*object_array[obj]).plane.normal[0] = value[0];
				(*object_array[obj]).plane.normal[1] = value[1];
				(*object_array[obj]).plane.normal[2] = value[2];
			}
		////////////////////////////////////////////////////////////
		// error ///////////////////////////////////////////////////
	  } else {
	    fprintf(stderr, "Error: Unknown property, \"%s\", on line %d.\n",
		    key, line);
	    //char* value = next_string(json);
	  }
	  skip_ws(json);
	} else {
	  fprintf(stderr, "Error: Unexpected value on line %d\n", line);
	  exit(1);
	}
      }
      skip_ws(json);
      c = next_c(json);
      if (c == ',') {
	// noop
	skip_ws(json);
      } else if (c == ']') {
	fclose(json);
	object_array[obj] = NULL;
	return;
      } else {
	fprintf(stderr, "Error: Expecting ',' or ']' on line %d.\n", line);
	exit(1);
      }
    }
  }
}

// Plymorphism in C

static inline double sqr(double v) {
  return v*v;
}


static inline void normalize(double* v) {
  double len = sqrt(sqr(v[0]) + sqr(v[1]) + sqr(v[2]));
  v[0] /= len;
  v[1] /= len;
  v[2] /= len;
}


double cylinder_intersection(double* Ro, double* Rd,
			     double* C, double r) {
  // Step 1. Find the equation for the object you are
  // interested in..  (e.g., cylinder)
  //
  // x^2 + z^2 = r^2
  //
  // Step 2. Parameterize the equation with a center point
  // if needed
  //
  // (x-Cx)^2 + (z-Cz)^2 = r^2
  //
  // Step 3. Substitute the eq for a ray into our object
  // equation.
  //
  // (Rox + t*Rdx - Cx)^2 + (Roz + t*Rdz - Cz)^2 - r^2 = 0
  //
  // Step 4. Solve for t.
  //
  // Step 4a. Rewrite the equation (flatten).
  //
  // -r^2 +
  // t^2 * Rdx^2 +
  // t^2 * Rdz^2 +
  // 2*t * Rox * Rdx -
  // 2*t * Rdx * Cx +
  // 2*t * Roz * Rdz -
  // 2*t * Rdz * Cz +
  // Rox^2 -
  // 2*Rox*Cx +
  // Cx^2 +
  // Roz^2 -
  // 2*Roz*Cz +
  // Cz^2 = 0
  //
  // Steb 4b. Rewrite the equation in terms of t.
  //
  // t^2 * (Rdx^2 + Rdz^2) +
  // t * (2 * (Rox * Rdx - Rdx * Cx + Roz * Rdz - Rdz * Cz)) +
  // Rox^2 - 2*Rox*Cx + Cx^2 + Roz^2 - 2*Roz*Cz + Cz^2 - r^2 = 0
  //
  // Use the quadratic equation to solve for t..
  double a = (sqr(Rd[0]) + sqr(Rd[2]));
  double b = (2 * (Ro[0] * Rd[0] - Rd[0] * C[0] + Ro[2] * Rd[2] - Rd[2] * C[2]));
  double c = sqr(Ro[0]) - 2*Ro[0]*C[0] + sqr(C[0]) + sqr(Ro[2]) - 2*Ro[2]*C[2] + sqr(C[2]) - sqr(r);

  double det = sqr(b) - 4 * a * c;
  if (det < 0) return -1;

  det = sqrt(det);
  
  double t0 = (-b - det) / (2*a);
  if (t0 > 0) return t0;

  double t1 = (-b + det) / (2*a);
  if (t1 > 0) return t1;

  return -1;
}

double sphere_intersection(double* Ro, double* Rd,
			     double* C, double r) {

	// Step 1:
	// x^2 + y^2 + z^2 = r^2
	// 
	// Step 2:
	// (x-Cx)^2 + (y-Cy)^2 + (z-Cz)^2 = r^2
	//
	// Step ?:
	// x^2 -2Cx +Cx^2 + y^2 -2Cy +Cy^2 + z^2 -2Cz +Cz^2 = r^2
	// (Rox + tRdx)^2 + (Roy + tRdy)^2 + (Roz + tRdz)^2 = r^2 + 2Cx + 2Cy + 2Cz - Cx^2 - Cy^2 - Cz^2
	// Rox^2 + 2RoxtRdx + tRdx^2 + Roy^2 + 2RoytRdy + tRdy^2 + Roz^2 + 2RoztRdz + tRdz^2 = r^2 + 2Cx + 2Cy + 2Cz - Cx^2 - Cy^2 - Cz^2
	// 2RoxtRdx + tRdx^2 + 2RoytRdy + tRdy^2 + 2RoztRdz + tRdz^2 = r^2 + 2Cx + 2Cy + 2Cz - Cx^2 - Cy^2 - Cz^2 - Rox^2 - Roy^2 - Roz^2
	// t^2(Rdx^2 + Rdy^2 + Rdz^2) + t(2RoxRdx + 2RoyRdy + 2RozRdz) - (r^2 + 2Cx + 2Cy + 2Cz - Cx^2 - Cy^2 - Cz^2 - Rox^2 - Roy^2 - Roz^2) = 0

	double a = (sqr(Rd[0]) + sqr(Rd[1]) + sqr(Rd[2]));
	double b = (2*(Ro[0]*Rd[0] - Rd[0]*C[0] + Ro[1]*Rd[1] - Rd[1]*C[1] + Ro[2]*Rd[2] - Rd[2]*C[2]));
	double c = sqr(Ro[0]) - 2*Ro[0]*C[0] + sqr(C[0]) + Ro[1] - 2*Ro[1]*C[1] + sqr(C[1]) + sqr(Ro[2]) - 2*Ro[2]*C[2] + sqr(C[2]) - sqr(r);

  double det = sqr(b) - 4 * a * c;
  if (det < 0) return -1;

  det = sqrt(det);
  
  double t0 = (-b - det) / (2*a);
  if (t0 > 0) return t0;

  double t1 = (-b + det) / (2*a);
  if (t1 > 0) return t1;

  return -1;

}

double plane_intersection(double* Ro, double* Rd,
			     double* C, double* N) {

	double t;
	
	return -1;
}

int main(int argc, char **argv) {

	FILE* output;
	output = fopen(argv[4], "wb+");
	fprintf(output, "P3\n");
	fprintf(output, "%d %d\n%d\n", atoi(argv[1]), atoi(argv[2]), 1);

  read_scene(argv[3]);
	int i = 0;
	double w;
	double h;
	while(1){
		if (object_array[i]->kind == 0){
			w = object_array[i]->camera.width;
  		h = object_array[i]->camera.height;
			printf("Camera found and variables set.\n");
			break;
		}
		i++;
	}
  
	double cx = 0;
  double cy = 0;

  int M = atoi(argv[2]);
  int N = atoi(argv[1]);

  double pixheight = h / M;
  double pixwidth = w / N;

	double* color;

	int j = 0;
	
  for (int y = 0; y < M; y += 1) {
    for (int x = 0; x < N; x += 1) {
      double Ro[3] = {0, 0, 0};
      // Rd = normalize(P - Ro)
      double Rd[3] = {
				cx - (w/2) + pixwidth * (x + 0.5),
				cy - (h/2) + pixheight * (y + 0.5),
				1
      };
      normalize(Rd);
      double best_t = INFINITY;
      for (int i=0; object_array[i] != 0; i++) {
				j = i;
				//printf("in loop\n");
				double t = 0;
				switch(object_array[i]->kind) {
				case 0:
	  			// pass, its a camera
	  			break;
				case 1:
					//printf("sphere int..\n");
	  			t = sphere_intersection(Ro, Rd,
				    	object_array[i]->sphere.position,
				    	object_array[i]->sphere.radius);
	  			break;
				case 2:
					//printf("plane int..\n");
	  			t = plane_intersection(Ro, Rd,
				    	object_array[i]->plane.position,
				    	object_array[i]->plane.normal);
	  			break;
				default:
	  			// Horrible error
	  			exit(1);
				}
				if (t > 0 && t < best_t) {
					best_t = t;
					color = object_array[i]->color;
				}
			}
			Pixel new;
    	if (best_t > 0 && best_t != INFINITY) {
				new.red = color[0];
				new.green = color[1];
				new.blue = color[2];
				fprintf(output, "%i %i %i ", new.red, new.green, new.blue);
    	} else {
				fprintf(output, "0 0 0 ");
    	}
    }
  }
  fclose(output);
  return 0;
}
