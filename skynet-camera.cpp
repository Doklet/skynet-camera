/*
 * http://github.com/dusty-nv/jetson-inference
 */

#include "gstCamera.h"

#include "glDisplay.h"
#include "glTexture.h"

#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#include <iostream>
#include <thread>
#include <chrono>
#include <pthread.h>

#include "CommHandler.h"
#include "DigitalIO.h"

#include "cudaNormalize.h"
#include "cudaFont.h"
#include "imageNet.h"
#include "loadImage.h"


#define DEFAULT_CAMERA -1	// -1 for onboard camera, or change to index of /dev/video V4L2 camera (>=0)	
		
		
bool signal_recieved = false;

void sig_handler(int signo)
{
	if( signo == SIGINT )
	{
		printf("received SIGINT\n");
		signal_recieved = true;
	}
}

void *spawnCommHandler(void * arg) {

	CommandHandler *commandHandler = (CommandHandler*)arg;

	CommHandler* comm = new CommHandler(commandHandler);

	comm->Open(3456);

	comm->Listen();

	delete comm;

	return NULL;
}

int main( int argc, char** argv )
{
	printf("imagenet-camera\n  args (%i):  ", argc);

	for( int i=0; i < argc; i++ )
		printf("%i [%s]  ", i, argv[i]);
		
	printf("\n\n");

	/* Set up command handler in wown thread*/

	CommandHandler* commandHandler = new CommandHandler();
	
	DigitalIO io;

	pthread_t t;
	
	pthread_create(&t, NULL, spawnCommHandler, (void *)commandHandler);

	std::cout << "Await commands \n" << std::endl;
	

	/* setup the IO's */
	io.Init();
	
	/*
	 * attach signal handler
	 */
	if( signal(SIGINT, sig_handler) == SIG_ERR )
		printf("\ncan't catch SIGINT\n");


	/*
	 * create the camera device
	 */
	gstCamera* camera = gstCamera::Create(DEFAULT_CAMERA);
	
	if( !camera )
	{
		printf("\nimagenet-camera:  failed to initialize video device\n");
		return 0;
	}
	
	printf("\nimagenet-camera:  successfully initialized video device\n");
	printf("    width:  %u\n", camera->GetWidth());
	printf("   height:  %u\n", camera->GetHeight());
	printf("    depth:  %u (bpp)\n\n", camera->GetPixelDepth());
	

	/*
	 * create imageNet
	 */
	imageNet* net = imageNet::Create(argc, argv);
	
	if( !net )
	{
		printf("imagenet-console:   failed to initialize imageNet\n");
		return 0;
	}


	/*
	 * create openGL window
	 */
	glDisplay* display = glDisplay::Create();
	glTexture* texture = NULL;
	
	if( !display ) {
		printf("\nimagenet-camera:  failed to create openGL display\n");
	}
	else
	{
		texture = glTexture::Create(camera->GetWidth(), camera->GetHeight(), GL_RGBA32F_ARB/*GL_RGBA8*/);

		if( !texture )
			printf("imagenet-camera:  failed to create openGL texture\n");
	}
	
	
	/*
	 * create font
	 */
	cudaFont* font = cudaFont::Create();
	

	/*
	 * start streaming
	 */
	if( !camera->Open() )
	{
		printf("\nimagenet-camera:  failed to open camera for streaming\n");
		return 0;
	}
	
	printf("\nimagenet-camera:  camera open for streaming\n");
	
	
	/*
	 * processing loop
	 */
	float confidence = 0.0f;

	/*
	 * set system ready output
	 */
	io.ClearSystemReady();
	io.ClearGood();
	io.ClearBad();	

	io.SetSystemReady();
	
	while( !signal_recieved )
	{
		void* imgCPU  = NULL;
		void* imgCUDA = NULL;
		
		// get the latest frame
		if( !camera->Capture(&imgCPU, &imgCUDA, 1000) )
			printf("\nimagenet-camera:  failed to capture frame\n");
		//else
		//	printf("imagenet-camera:  recieved new frame  CPU=0x%p  GPU=0x%p\n", imgCPU, imgCUDA);
		
		// convert from YUV to RGBA
		void* imgRGBA = NULL;
		
		if( !camera->ConvertRGBA(imgCUDA, &imgRGBA, true) )
			printf("imagenet-camera:  failed to convert from NV12 to RGBA\n");

		// classify image
		const int img_class = net->Classify((float*)imgRGBA, camera->GetWidth(), camera->GetHeight(), &confidence);

		/*
		 * Handle any incomming command  
		 */
		if (Command* cmd = commandHandler->GetNextCommand()) {
			switch (cmd->type) {
				case RECORD:
				{
					std::cout << "Got a record command\n";
					//std::cout << "recordPath: " << cmd->recordCommand->recordPath << std::endl;
					std::cout << "Before check\n";
					const char *outputfile = "skynet_output.jpg";
					if( !saveImageRGBA(outputfile, (float4*)imgRGBA, camera->GetWidth(), camera->GetHeight()) )
						printf("imagenet-console:  failed to save output image to '%s'\n", outputfile);
					else
						printf("imagenet-console:  completed saving '%s'\n", outputfile);
					break;
					break;
				}
				case PREDICT:
				{
					std::cout << "Got a predict command:\n" << std::endl;
					std::cout << "filePath: " << cmd->predictCommand->filePath << "\n";
					break;
				}
				default:
					std::cout << "Got a unkown command" << "\n";
			}

			// TODO need to fix this, crash on TX1
			// delete cmd;
		}
	
		if( img_class >= 0 )
		{
			// printf("imagenet-camera:  %2.5f%% class #%i (%s)\n", confidence * 100.0f, img_class, net->GetClassDesc(img_class));	

			if( font != NULL )
			{
				char str[256];
				sprintf(str, "%05.2f%% %s", confidence * 100.0f, net->GetClassDesc(img_class));
	
				font->RenderOverlay((float4*)imgRGBA, (float4*)imgRGBA, camera->GetWidth(), camera->GetHeight(),
								    str, 0, 0, make_float4(0.0f, 255.0f, 255.0f, 255.0f));
			}
			
			
			if( display != NULL )
			{
				char str[256];
				sprintf(str, "TensorRT build %x | %s | %s | %04.1f FPS", NV_GIE_VERSION, net->GetNetworkName(), net->HasFP16() ? "FP16" : "FP32", display->GetFPS());
				//sprintf(str, "TensorRT build %x | %s | %04.1f FPS | %05.2f%% %s", NV_GIE_VERSION, net->GetNetworkName(), display->GetFPS(), confidence * 100.0f, net->GetClassDesc(img_class));
				display->SetTitle(str);	
			}
			
			// Set the correct io's
			if (img_class == 0) {
				io.SetGood();
				io.ClearBad();	
			} else if (img_class == 1) {
				io.ClearGood();
				io.SetBad();	
			} else {
				io.ClearGood();
				io.ClearBad();	
			}
		}	


		// update display
		if( display != NULL )
		{
			display->UserEvents();
			display->BeginRender();

			if( texture != NULL )
			{
				// rescale image pixel intensities for display
				CUDA(cudaNormalizeRGBA((float4*)imgRGBA, make_float2(0.0f, 255.0f), 
								   (float4*)imgRGBA, make_float2(0.0f, 1.0f), 
		 						   camera->GetWidth(), camera->GetHeight()));

				// map from CUDA to openGL using GL interop
				void* tex_map = texture->MapCUDA();

				if( tex_map != NULL )
				{
					cudaMemcpy(tex_map, imgRGBA, texture->GetSize(), cudaMemcpyDeviceToDevice);
					texture->Unmap();
				}

				// draw the texture
				texture->Render(100,100);		
			}

			display->EndRender();
		}


	}
	
	io.ClearSystemReady();
	io.ClearGood();
	io.ClearBad();	

	printf("\nimagenet-camera:  un-initializing video device\n");
	
	
	/*
	 * shutdown the camera device
	 */
	if( camera != NULL )
	{
		delete camera;
		camera = NULL;
	}

	if( display != NULL )
	{
		delete display;
		display = NULL;
	}
	
	/* shutdown the IO's */
	io.Shutdown();
	
	printf("imagenet-camera:  video device has been un-initialized.\n");
	printf("imagenet-camera:  this concludes the test of the video device.\n");
	return 0;
}

