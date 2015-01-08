#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <CL/opencl.h>

const char *histogram = "\
__kernel void histogram(\
	__global unsigned char *image_data,\
	unsigned int count,\
	__global unsigned int *result_data)\
{\
	const int idx = get_global_id(0);\
	const int idy = get_global_id(1);\
\
	unsigned int i;\
	for(i=0; i<count; i++) {\
		if(idx == image_data[i * 3 + idy]) {\
			result_data[256 * idy + idx]++;\
		}\
	}\
}\
";

int main(int argc, char const *argv[])
{
	cl_int clErrNum;
	cl_uint num_device, ret_num_platforms;
	cl_device_id device;
	cl_platform_id platform_id;

	// Initialization
	clErrNum = clGetPlatformIDs(1, &platform_id, &ret_num_platforms);
	clErrNum = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_GPU, 1, &device,  &num_device);
	cl_context myctx = clCreateContext(NULL, 1, &device, NULL, NULL, &clErrNum);
	cl_command_queue myque = clCreateCommandQueue(myctx, device, 0, &clErrNum);

	// Open file
	FILE *inFile = fopen("input", "r");
	FILE *outFile = fopen("0116229.out", "w");

	unsigned int i = 0, a, input_size;

	fscanf(inFile, "%u", &input_size);

	// Allocate memory
	cl_mem img_in = clCreateBuffer(myctx, CL_MEM_READ_ONLY, sizeof(unsigned char) * input_size, NULL, &clErrNum);
	cl_mem his_out = clCreateBuffer(myctx, CL_MEM_WRITE_ONLY, sizeof(unsigned int) * 256 * 3, NULL, &clErrNum);
	//clErrNum = clEnqueueFillBuffer(myque, his_out, "\0", 1, 0, sizeof(unsigned int) * 256 * 3, NULL, NULL, NULL);
	unsigned char *tmp_image = (unsigned char *) malloc(sizeof(unsigned char) * input_size);
	unsigned int *result_his = (unsigned int *) malloc(sizeof(unsigned int) * 256 * 3);

	memset(result_his, 0x00, sizeof(unsigned int) * 256 * 3);
	clEnqueueWriteBuffer(myque, his_out, CL_TRUE, 0, sizeof(unsigned int) * 256 * 3, result_his, 0, NULL, NULL);

	// Read input
	while( fscanf(inFile, "%u", &a) != EOF ) {
		tmp_image[i++] = a;
	}
	clErrNum = clEnqueueWriteBuffer(myque, img_in, CL_TRUE, 0, sizeof(unsigned char) * input_size, tmp_image, 0, NULL, NULL);

	size_t source_size = strlen(histogram);
	unsigned int input_count = input_size/3;
	cl_program myprog = clCreateProgramWithSource(myctx, 1, (const char **) &histogram, &source_size, &clErrNum);
	clErrNum = clBuildProgram(myprog, 1, &device, NULL, NULL, NULL);
	cl_kernel mykernel = clCreateKernel(myprog, "histogram", &clErrNum);
	clSetKernelArg(mykernel, 0, sizeof(cl_mem), &img_in);
	clSetKernelArg(mykernel, 1, sizeof(unsigned int), &input_count);
	clSetKernelArg(mykernel, 2, sizeof(cl_mem), &his_out);

	size_t global_ws[2] = {256, 3};
	size_t local_ws[2] = {1, 1};
	clErrNum = clEnqueueNDRangeKernel(myque, mykernel, 2, 0, global_ws, local_ws, 0, NULL, NULL);

	clErrNum = clEnqueueReadBuffer(myque, his_out, CL_TRUE, 0, sizeof(unsigned int) * 256 * 3, result_his, NULL, NULL, NULL);
	for(i = 0; i < 256 * 3; ++i) {
		if (i % 256 == 0 && i != 0)
			fprintf(outFile, "\n");
		fprintf(outFile, "%u ", result_his[i]);
	}

	fclose(inFile);
	fclose(outFile);

	return 0;
}
