// Imagine++ project
// Project:  Deblur
// Author:   Yohann Salaun
// Date:     2012/12/25

#include <Imagine/Images.h>
#include <Imagine/Graphics.h>
#include <vector>
#include <Imagine/LinAlg.h>

using namespace Imagine;
using namespace std;

// Color operations
DoublePoint3 point(Color c){
	return DoublePoint3(double(c.r()/255.0), double(c.g()/255.0), double(c.b()/255.0));
}

 Color color(DoublePoint3 p){
	return Color(int(255*p.x()), int(255*p.y()), int(255*p.z()));
}

Color add(Color c1, Color c2){
	return Color(c1.r()+c2.r(), c1.g()+c2.g(), c1.b()+c2.b()); 
}

Color times(Color c1, float x){
	return Color(c1.r()*x, c1.g()*x, c1.b()*x); 
}

DoublePoint3 times(DoublePoint3 p1, DoublePoint3 p2){
	return DoublePoint3(p1.x()*p2.x(), p1.y()*p2.y(), p1.z()*p2.z());
}

DoublePoint3 divide(DoublePoint3 p1, DoublePoint3 p2){
	return DoublePoint3(p1.x()/p2.x(), p1.y()/p2.y(), p1.z()/p2.z());
}

// intensity functions ---------------------------------------
Color color(const Image<Color,2>& I, const int i, const int j){
	if(i<0 || i>=I.width() || j<0 || j>=I.height()){
		return Color(0,0,0);
	}
	return I(i,j);
}

float color2gray(const Color c){
	return (c.r()/3.0+c.g()/3.0+c.b()/3.0)/256.0;
}

// display functions -----------------------------------------
void displayMatrix(const Matrix<float> &A, const int w, const int h){
	int i,j;
	for(i=0; i<w; ++i){
		for(j=0; j<h; ++j){
			cout << A[i+j*w] << " ";
		}
		cout << endl;
	}
}

void displayVector(const Vector<float> &A){
	int i;
	for(i=0; i<A.size(); ++i){
		cout << A[i] << endl;
	}
}

void displayKernel(const Vector<float> &k){
	int i,j;
	int kernel_size = int(sqrt(float(k.size())));

	for(i=0; i<kernel_size; ++i){
		for(j=0; j<kernel_size; ++j){
			cout << k[i+j*kernel_size] << " ";
		}
		cout << endl;
	}
}

// conversions functions ------------------------------------
Vector<float> img2vect(const Image<Color,2>& I){
	int i, j, w = I.width(), h = I.height();
	Vector<float> If;
	If.setSize(w*h);

	for(i=0; i<w; ++i){
		for(j=0; j<h; ++j){
			If[i+j*w] = color2gray(I(i,j));
		}
	}
	
	return If;
}

Image<Color,2> vect2img(const Vector<float>& I, const int w, const int h){
	int i, j, c;
	Image<Color,2> Ic(w, h);

	for(i=0; i<w; ++i){
		for(j=0; j<h; ++j){
			c = int(256*I[i+j*w]);
			Ic(i,j) = Color(c, c, c);
		}
	}
	
	return Ic;
}

Matrix<float> img2kernelMatrix(const Image<Color,2>& I, const int kernel_size){
	int i, j, ii, jj, w = I.width(), h = I.height();
	Matrix<float> Imf;
	Imf.setSize(w*h, kernel_size*kernel_size);
	int ks = kernel_size/2;

	for(i=0; i<w; ++i){
		for(j=0; j<h; ++j){
			for(ii=-ks; ii<=ks; ++ii){
				for(jj=-ks; jj<=ks; ++jj){
					Imf(i+j*w, ii+ks+(jj+ks)*kernel_size) = color2gray(color(I, i+ii, j+jj));
				}
			}
		}
	}
	
	return Imf;
}

// convolution with kernel function -------------------------
Image<Color,2> kernelBlurring(const Image<Color,2>& I, const Vector<float>& k){
	int i, j, ii, jj;
	int w = I.width(), h = I.height();
	int kernel_size = int(sqrt(float(k.size())));
	int ks = kernel_size/2;
	Color c;
	DoublePoint3 sum;
	Image<Color,2> Ik(w, h);

	for(i=0; i<w; ++i){
		for(j=0; j<h; ++j){
			sum = DoublePoint3(0, 0, 0);
			for(ii=-ks; ii<=ks; ++ii){
				for(jj=-ks; jj<=ks; ++jj){
					c = color(I, i+ii, j+jj);
					sum += point(c)*k[ii+ks+(jj+ks)*kernel_size];
				}
			}
			Ik(i,j) = color(sum);
		}
	}
	return Ik;
}


// algorithm ------------------------------------------------

// projection for kernel estimation part
void projection(Vector<float>& k){
	int i;
	float norm = 0;

	for(i=0; i<k.size(); ++i){
		k[i] = max(k[i], float(0));
		norm += k[i];
	}
	if(norm > 0)
	{
		for(i=0; i<k.size(); ++i){
			k[i] /= norm;
		}
	}
}

// kernel estimation
Vector<float> kernelEstimation(const Image<Color,2>& I, const Image<Color,2>& B, const int kernel_size){
	int i, j;
	float k_size2 = kernel_size*kernel_size;
	float norm;

	// parameters
	int niter = 100000;						// number of iterations
	float beta = 0.000001;					// parameter for reccursive formula of k
	float lambda = 1;						// smoothness parameter
	float lambda2 = lambda*lambda;
	
	// Vector_Matrix form
	Vector<float> b = img2vect(B);
	Vector<float> k;
	k.setSize(kernel_size*kernel_size);
	Matrix<float> A = img2kernelMatrix(I, kernel_size);
	Matrix<float> AtA = transpose(A)*A;
	Vector<float> Atb = transpose(A)*b;

	// initialize k as a dirac
	for(i=0; i<kernel_size*kernel_size; ++i){
		k[i] = 0;
	}
	k[kernel_size/2*(1 + kernel_size)] = 1;

	// loop to find k
	for(i=0; i<niter; ++i){
		k = k + beta*(Atb -	AtA*k - lambda2*k);
	}

	projection(k);

	return k;
}

// deconvolution algorithm
Color RLdeconv(const Image<Color,2>& u, const Image<Color,2>& b, const Image<Color,2>& c, const Vector<float> &k, const int x, const int y){
	int kernel_size = int(sqrt(float(k.size())));
	int i, j, I, J;
	float kij, offset = 0.0;
	DoublePoint3 b_col, c_col, color_sum;

	color_sum = DoublePoint3(0, 0, 0);
	for(i = 0; i<kernel_size; ++i){
		for(j = 0; j<kernel_size; ++j){
			I = i - kernel_size/2;
			J = j - kernel_size/2;
			
			kij = k[-I+kernel_size/2+(-J+kernel_size/2)*kernel_size];
			b_col = point(color(b, x+I, y+J))+offset;
			c_col = point(color(c, x+I, y+J))+offset;
			
			color_sum += kij*divide(b_col, c_col);
		}
	}

	return color(times(point(color(u, x, y))+offset, color_sum)-offset);
}

void deconvol(Image<Color,2>& I, const Image<Color,2>& B, const Image<Color,2>& Nd, const Vector<float>& k){
	int kernel_size = int(sqrt(float(k.size())));
	int niter = 10;
	int i, x, y, w = I.width(), h = I.height();

	// initialization of delta_pictures
	Image<Color,2> deltaI = I; - Nd;
	Image<Color,2> deltaB = B; - kernelBlurring(Nd, k);
	Image<Color,2> deltaInext(w, h);
	Image<Color,2> deltaIblurred(w, h);
	
	for(i = 0; i<niter; ++i){
		// blur delta I
		deltaIblurred = kernelBlurring(deltaI, k);
		// compute new delta I
		for(x = 0; x<w; ++x){
			for(y = 0; y<h; ++y){
				deltaInext(x, y) = RLdeconv(deltaI, deltaB, deltaIblurred, k, x, y);
			}
		}
		// update
		for(x = 0; x<w; ++x){
			for(y = 0; y<h; ++y){
				deltaI(x, y) = deltaInext(x, y);
			}
		}
	}

}

// main algorithm
Vector<float> deblur(Image<Color,2>& I, const Image<Color,2>& B, const Image<Color,2>& Nd,const int kernel_size, const Vector<float> &kernel){
	int i;
	int niter = 10;
	Vector<float> estimated_kernel;

	// display real kernel
	displayKernel(kernel);

	for(i = 0; i<niter; ++i){
		// estimate kernel with current picture
		estimated_kernel = kernelEstimation(I, B, kernel_size);

		// display estimated kernel
		cout << "-- Iteration " << i << " ---" << endl;
		displayKernel(estimated_kernel);
		
		// display norm 2 of difference
		cout << "norm difference: " << norm2(estimated_kernel-kernel)/norm2(kernel) << endl;

		// deconvolution of blurred picture
		deconvol(I, B, Nd, estimated_kernel);
	}
	return estimated_kernel;
}

// main -----------------------------------------------------
int main()
{
	// Load and display images
	Image<Color,2> B, estimated_blur, Nd, original, I;
	if( ! load(B, srcPath("lena_blurred.jpg")) ||
        ! load(Nd, srcPath("lena_denoised.jpg")) ||
		! load(original, srcPath("lena.jpg")) ||
		! load(I, srcPath("lena_denoised.jpg"))) {
        cerr<< "Unable to load images" << endl;
        return 1;
    }
	int w = B.width();
	int h = B.height();

	int kernel_size = 5;

	// create blur kernel
	int ii, jj;
	Vector<float> kernel, estimated_kernel;
	kernel.setSize(kernel_size*kernel_size);
	int ks = kernel_size/2;
	float norm = 0;
	
	// initialize k as a gaussian
	for(ii=-ks; ii<=ks; ++ii){
		for(jj=-ks; jj<=ks; ++jj){
			kernel[ii+ks+(jj+ks)*kernel_size] = rand();
			kernel[ii+ks+(jj+ks)*kernel_size] = exp(-float(ii*ii+jj*jj));
			norm += kernel[ii+ks+(jj+ks)*kernel_size];
		}
	}
	kernel /= norm;

	// compute blurred picture
	B = kernelBlurring(original, kernel);

	// deblur picture
	estimated_kernel = deblur(I, B, Nd, kernel_size, kernel);
	estimated_blur = kernelBlurring(I, estimated_kernel);

	// display window
	openWindow(2*w, 2*h);
	display(original,0,0);
	display(I,w,0);	
	display(B,0,h);
	display(estimated_blur,w,h);
	

	endGraphics();
	return 0;
}
