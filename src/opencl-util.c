#include "opencl-util.h"

void ocl_strerror(cl_int err, char* buf, size_t buflen) {
	const char *msg;
	switch(err) {
		case CL_SUCCESS:                            msg = "Success!"; break;
		case CL_DEVICE_NOT_FOUND:                   msg = "Device not found."; break;
		case CL_DEVICE_NOT_AVAILABLE:               msg = "Device not available"; break;
		case CL_COMPILER_NOT_AVAILABLE:             msg = "Compiler not available"; break;
		case CL_MEM_OBJECT_ALLOCATION_FAILURE:      msg = "Memory object allocation failure"; break;
		case CL_OUT_OF_RESOURCES:                   msg = "Out of resources"; break;
		case CL_OUT_OF_HOST_MEMORY:                 msg = "Out of host memory"; break;
		case CL_PROFILING_INFO_NOT_AVAILABLE:       msg = "Profiling information not available"; break;
		case CL_MEM_COPY_OVERLAP:                   msg = "Memory copy overlap"; break;
		case CL_IMAGE_FORMAT_MISMATCH:              msg = "Image format mismatch"; break;
		case CL_IMAGE_FORMAT_NOT_SUPPORTED:         msg = "Image format not supported"; break;
		case CL_BUILD_PROGRAM_FAILURE:              msg = "Program build failure"; break;
		case CL_MAP_FAILURE:                        msg = "Map failure"; break;
		case CL_INVALID_VALUE:                      msg = "Invalid value"; break;
		case CL_INVALID_DEVICE_TYPE:                msg = "Invalid device type"; break;
		case CL_INVALID_PLATFORM:                   msg = "Invalid platform"; break;
		case CL_INVALID_DEVICE:                     msg = "Invalid device"; break;
		case CL_INVALID_CONTEXT:                    msg = "Invalid context"; break;
		case CL_INVALID_QUEUE_PROPERTIES:           msg = "Invalid queue properties"; break;
		case CL_INVALID_COMMAND_QUEUE:              msg = "Invalid command queue"; break;
		case CL_INVALID_HOST_PTR:                   msg = "Invalid host pointer"; break;
		case CL_INVALID_MEM_OBJECT:                 msg = "Invalid memory object"; break;
		case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:    msg = "Invalid image format descriptor"; break;
		case CL_INVALID_IMAGE_SIZE:                 msg = "Invalid image size"; break;
		case CL_INVALID_SAMPLER:                    msg = "Invalid sampler"; break;
		case CL_INVALID_BINARY:                     msg = "Invalid binary"; break;
		case CL_INVALID_BUILD_OPTIONS:              msg = "Invalid build options"; break;
		case CL_INVALID_PROGRAM:                    msg = "Invalid program"; break;
		case CL_INVALID_PROGRAM_EXECUTABLE:         msg = "Invalid program executable"; break;
		case CL_INVALID_KERNEL_NAME:                msg = "Invalid kernel name"; break;
		case CL_INVALID_KERNEL_DEFINITION:          msg = "Invalid kernel definition"; break;
		case CL_INVALID_KERNEL:                     msg = "Invalid kernel"; break;
		case CL_INVALID_ARG_INDEX:                  msg = "Invalid argument index"; break;
		case CL_INVALID_ARG_VALUE:                  msg = "Invalid argument value"; break;
		case CL_INVALID_ARG_SIZE:                   msg = "Invalid argument size"; break;
		case CL_INVALID_KERNEL_ARGS:                msg = "Invalid kernel arguments"; break;
		case CL_INVALID_WORK_DIMENSION:             msg = "Invalid work dimension"; break;
		case CL_INVALID_WORK_GROUP_SIZE:            msg = "Invalid work group size"; break;
		case CL_INVALID_WORK_ITEM_SIZE:             msg = "Invalid work item size"; break;
		case CL_INVALID_GLOBAL_OFFSET:              msg = "Invalid global offset"; break;
		case CL_INVALID_EVENT_WAIT_LIST:            msg = "Invalid event wait list"; break;
		case CL_INVALID_EVENT:                      msg = "Invalid event"; break;
		case CL_INVALID_OPERATION:                  msg = "Invalid operation"; break;
		case CL_INVALID_GL_OBJECT:                  msg = "Invalid OpenGL object"; break;
		case CL_INVALID_BUFFER_SIZE:                msg = "Invalid buffer size"; break;
		case CL_INVALID_MIP_LEVEL:                  msg = "Invalid mip-map level"; break;
		default: msg = "Unknown"; break;
	}
	snprintf(buf, buflen, "[%d]: %s", err, msg);
}

char _ocl_errbuf[OCL_ERRBUF_SIZE];
