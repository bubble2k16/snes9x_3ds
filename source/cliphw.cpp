
#define _CLIPHW_CPP_
#include "cliphw.h"




#define CLIP_OR 0
#define CLIP_AND 1
#define CLIP_XOR 2
#define CLIP_XNOR 3




void ComputeClipWindowsForStenciling (
	uint32 window1Left, uint32 window1Right, uint32 window2Left, uint32 window2Right, 
	int *endX, int *stencilMask)
{
	int sectionCount = 0;

	window1Right ++;
	window2Right ++;
	#define INSERT_SECTION(x,w1,w2) \
		if ((x) != 0) \
		{ \
			if (sectionCount == 0 || endX[sectionCount - 1] != (x)) \
			{ \
				stencilMask[sectionCount] = 0; \
				stencilMask[sectionCount] += (w1 == 1) ? WIN1_MASK : 0; \
				stencilMask[sectionCount] += (w2 == 1) ? WIN2_MASK : 0; \
				\
				int xw1 = w1; if (w1 == -1) xw1 = 0; \
				int xw2 = w2; if (w2 == -1) xw2 = 0; \
				\
				stencilMask[sectionCount] += (xw1 ^ xw2) ? WIN1XOR2_MASK : 0; \
				\
				endX[sectionCount] = (x); \
				sectionCount++; \
			} \
		} 

	// Find all the possible intersection points
	// 
	if (window1Left < window1Right)
	{
		if (window2Left < window2Right)
		{
			// sort the sections list
			//
			#define W1L_LESS_W2L   (1) 
			#define W1L_LESS_W2R   (2) 
			#define W1L_LESS_256   (3) 
			#define W1R_LESS_W2L   (1 << 2) 
			#define W1R_LESS_W2R   (2 << 2)
			#define W1R_LESS_256   (3 << 2) 

			int cond = 0;

			if (window1Left < window2Left) 
				cond = W1L_LESS_W2L; 
			else if (window1Left < window2Right) 
				cond = W1L_LESS_W2R;
			else 
				cond = W1L_LESS_256;

			if (window1Right < window2Left) 
				cond += W1R_LESS_W2L; 
			else if (window1Right < window2Right) 
				cond += W1R_LESS_W2R;
			else  
				cond += W1R_LESS_256;

			switch (cond)
			{
				case W1L_LESS_W2L + W1R_LESS_W2L:
					// W1     1L-------1R                      |
					// W2                    2L-------2R	   |
					INSERT_SECTION(window1Left,  0, 0); 
					INSERT_SECTION(window1Right, 1, 0); 
					INSERT_SECTION(window2Left,  0, 0); 
					INSERT_SECTION(window2Right, 0, 1); 
					INSERT_SECTION(256,              0, 0); 
					break;

				case W1L_LESS_W2L + W1R_LESS_W2R:
					// W1     1L-----------------1R            |
					// W2                    2L-------2R	   |
					INSERT_SECTION(window1Left,  0, 0); 
					INSERT_SECTION(window2Left,  1, 0); 
					INSERT_SECTION(window1Right, 1, 1); 
					INSERT_SECTION(window2Right, 0, 1); 
					INSERT_SECTION(256,              0, 0); 
					break;

				case W1L_LESS_W2L + W1R_LESS_256:
					// W1     1L--------------------------1R   |
					// W2                    2L-------2R	   |
					INSERT_SECTION(window1Left,  0, 0); 
					INSERT_SECTION(window2Left,  1, 0); 
					INSERT_SECTION(window2Right, 1, 1); 
					INSERT_SECTION(window1Right, 1, 0); 
					INSERT_SECTION(256,              0, 0); 
					break;

				case W1L_LESS_W2R + W1R_LESS_W2R:
					// W1          1L------1R                  |
					// W2     2L----------------2R	   		   |
					INSERT_SECTION(window2Left,  0, 0); 
					INSERT_SECTION(window1Left,  0, 1); 
					INSERT_SECTION(window1Right, 1, 1); 
					INSERT_SECTION(window2Right, 0, 1); 
					INSERT_SECTION(256,              0, 0); 
					break;

				case W1L_LESS_W2R + W1R_LESS_256:
					// W1          1L----------------1R        |
					// W2     2L----------------2R	   		   |
					INSERT_SECTION(window2Left,  0, 0); 
					INSERT_SECTION(window1Left,  0, 1); 
					INSERT_SECTION(window2Right, 1, 1); 
					INSERT_SECTION(window1Right, 1, 0); 
					INSERT_SECTION(256,              0, 0); 
					break;
				
				case W1L_LESS_256 + W1R_LESS_256:
					// W1                            1L---1R   |
					// W2     2L----------------2R	   		   |
					INSERT_SECTION(window2Left,  0, 0); 
					INSERT_SECTION(window2Right, 0, 1); 
					INSERT_SECTION(window1Left,  0, 0); 
					INSERT_SECTION(window1Right, 1, 0); 
					INSERT_SECTION(256,              0, 0); 
					break;
			}
		}
		else 
		{
			// W1     1L-------1R               |
			// W2         ???	      		    |
			INSERT_SECTION(window1Left,  0, -1); 
			INSERT_SECTION(window1Right, 1, -1); 
			INSERT_SECTION(256,              0, -1); 
		}
	}
	else
	{
		if (window2Left < window2Right)
		{
			// W1         ???	      		    |
			// W2     2L-------2R               |
			INSERT_SECTION(window2Left,  -1, 0); 
			INSERT_SECTION(window2Right, -1, 1); 
			INSERT_SECTION(256,              -1, 0); 
		}
		else
		{
			// W1         ???	      		    |
			// W2         ???	      		    |
			INSERT_SECTION(256,              -1, -1); 
		}
	}
}
