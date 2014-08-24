/* FrameDataDump.c from LIGOtools by Peter Shawhan */

/*remove for unix*/
/*#define _PC*/

#include <stdio.h>
#include <stdlib.h>
#include "FrameL.h"
#include <math.h>
#include <string.h>

/*Frames will be considered nonsequencial if the time between two frames is
  greater than this*/
#define RESOLUTION 3.2e-3

/*This will take care of type casting the samples and print sample to the screen*/
void printSample(int type, void *data, int i);

/*This is used to print an unformatted sample to standard out.
  This is used rather then printSample only is -a is provided for ASCII output*/
void printUnformattedSample(int type, void *data, int i);

/*This will type the data to short, then write length **samples** to outFile
  starting offset **bytes** into the original array*/
int writeShortInt(int type, char *data, int length, int offset, FILE *outFile);

/*Identical to writeShortInt, but it types to float*/
int writeFloat(int type, char *data, int length, int offset, FILE *outFile);

/*This will put the data type into the string typeStr*/
void getDataType(int type, char *typeStr);

/*This returns 1 if type is a know type or 0 if it is not*/
int isValidType(int type);

/*This function is used when there seems to be an error with the inputs*/
void showUsage()
{
	printf("----------\n");
	printf("FrameDataDump <options>\n");
	printf("----------- Notes -----------\n");
	printf("* This program will take a directory of frame files as input and write a binary\n");
	printf("  file of the time series to disk\n");
	printf("* The frame files must be named alphabetically in the correct temporal order\n");
	printf("* Options -I and -C are required; one of -O or -d also required\n");
	printf("* If no -N or -T parameter is given, it is equivalent to passing -N-1");
	printf("* Do not place any characters after the options, ie -N100000 is correct\n");
	printf("---------- Options ----------\n");
	printf("-I<path>     : The path to the frame directory as well as any file search\n");
	printf("               parameters or a single file name. If search parameters are used\n");
	printf("               the option must be in single quotes, ie -I'path/*'\n");
	printf("-O<path>     : The path to the file in which to output the data\n");
	printf("-C<channel>  : The channel to extract data from\n");
	printf("-N<samples>  : The number of samples to extract, -1 for all availible\n");
	printf("-T<seconds>  : The number of seconds of data to extract.  Use in lieu of -N\n");
	printf("-Sn<samples> : The number of samples to skip from the frame files before output\n");
	printf("-St<seconds> : The number of seconds of data to skip.  Use in lieu of -Sn\n");
	printf("-Sf<files>   : The number of complete frame files to skip before output\n");
	printf("-D<#> [file] : Output frame library debug info to file; if file is stdout,\n");
	printf("             : output will be to the screen; # = debug level = 1(min) to 5(max)\n");
	printf("-z           : Place zeros for any time gaps between frames\n");
	printf("-d           : Display sample values to the screen in lieu of writing a file\n");
	printf("-s           : Force short int's to be written to the binary file\n");
	printf("-f           : Force float's to be written to the binary file\n");
	printf("-a           : Supress all output to standard out except that of the time series\n");
	printf("               This is useful to output data to an ASCII file\n");
}

int main(int argc, char *argv[])
{
	char *channel, *framePath, *outPath, outPath2[256] = "", curFile[256], typeStr[15], *debugPath;
	char* tmpName = tmpnam(NULL);
	struct FrFile *iFile;
	struct FrameH *frame;
	struct FrVect *frvect;
	struct FrAdcData *adc;
	struct FrProcData *proc;
	struct FrSimData *sim;
	double srate = 0, timeStamp, prevStamp, curTime = 0, skipTime = 0;
	int Iflag = 0, Oflag = 0, Cflag = 0, Nflag = 0, Tflag = 0, zeroFlag = 0, displayFlag = 0;
	int i, debugLevel = -1, skipFiles = 0, skipPoints = 0, sPointCount = 0, AllFlag = 0;
	int intFlag = 0, floatFlag = 0, ASCIIflag = 0, debugFlag = 0;
	int fileNum = 0, frameNum = 0, nPoints = 0, pointCount = 0, gapCount = 0, wSize, nData, type;
	FILE *debug, *list, *outFile, *outInfo;

/*Get the inputs passed to the program*/
	if(argc < 4)
	{
		showUsage();
		exit(1);
	}

	for(i = 1; i < argc; i++)
	{
		if(*(argv[i]) != '-')
		{
			showUsage();
			fprintf(stderr, "Invalid option %s\n", argv[i]);
			exit(1);
		}
		switch(*(argv[i] + 1))
		{
			case 'I':
				framePath = argv[i] + 2;
				Iflag = 1;
				break;
			case 'O':
				outPath = argv[i] + 2;
				Oflag = 1;
				break;
			case 'C':
				channel = argv[i] + 2;
				Cflag = 1;
				break;
			case 'N':
				nPoints = atol(argv[i] + 2);
				if(nPoints == -1)
				{
					AllFlag = 1;
					nPoints = INT_MAX;
				}
				else if(nPoints <= 0)
				{
					showUsage();
					fprintf(stderr, "Invalid number of points\n");
					exit(1);
				}
				pointCount = nPoints;
				Nflag = 1;
				break;
			case 'T':
				if((curTime = atof(argv[i] + 2)) <= 0)
				{
					showUsage();
					fprintf(stderr, "Invalid time\n");
					exit(1);
				}
				pointCount = 1;	 /*Just to get us into the loop the first time around*/
				Tflag = 1;
				break;
			case 'S':
				if(*(argv[i] + 2) == 'n')
				{
					if((skipPoints = atol(argv[i] + 3)) <= 0)
					{
						showUsage();
						fprintf(stderr, "Invalid number of points to skip\n");
						exit(1);
					}
					sPointCount = skipPoints;
				}
				else if(*(argv[i] + 2) == 'f')
				{
					if((skipFiles = atol(argv[i] + 3)) <= 0)
					{
						showUsage();
						fprintf(stderr, "Invalid number of files to skip\n");
						exit(1);
					}
				}
				else if(*(argv[i] + 2) == 't')
				{
					if((skipTime = atof(argv[i] + 3)) <= 0)
					{
						showUsage();
						fprintf(stderr, "Invalid time to skip\n");
						exit(1);
					}
				}
				break;
			case 'D':
				debugLevel = atol(argv[i] + 2);
				if(debugLevel < 0)
					debugLevel = 0;
				i++;
				if((i >= argc) || (*argv[i] == '-') )
				{
					debugPath = malloc(strlen("debug.txt")+1*sizeof(char));
					strcpy(debugPath, "debug.txt");
					debugFlag = 1;
					i--;
				}
				else {
					debugPath = argv[i]; }
				break;
			case 'z':
				zeroFlag = 1;
				break;
			case 'd':
				displayFlag = 1;
				break;
			case 's':
				intFlag = 1;
				break;
			case 'f':
				floatFlag = 1;
				break;
			case 'a':
				ASCIIflag = 1;
				break;
			default:
				showUsage();
				fprintf(stderr, "Invalid option %s\n", argv[i]);
				exit(1);
		}
	}
	
	if(Nflag == 0 && Tflag == 0)  /*sets number of points to all if no value given*/
	{
		Nflag = 1;
		AllFlag = 1;
		nPoints = INT_MAX;
		pointCount = nPoints;
	}

	if(Nflag == 1 && Tflag == 1)
	{
		fprintf(stderr, "\nCannot provide -N and -T options simultaneously\n");
		exit(1);
	}

	if(skipPoints > 0 && skipTime > 0)
	{
		fprintf(stderr, "\nCannot provide -Sn and -St options simultaneously\n");
		exit(1);
	}

	if(intFlag == 1 && floatFlag == 1)
	{
		fprintf(stderr, "\nCannot provide -s and -f options simultaneously\n");
		exit(1);
	}

	if(displayFlag == 1 && (intFlag == 1 || floatFlag == 1))
	{
		fprintf(stderr, "\nCannot provide -d and -f or -s options simultaneously\n");
		exit(1);
	}

	if(ASCIIflag == 1 && displayFlag == 0)
	{
		fprintf(stderr, "\nCannot provide -a option without -d option\n");
		exit(1);
	}

	if(zeroFlag == 1 && displayFlag == 1)
	{
		fprintf(stderr, "\nCannot provide -z and -d options simultaneously\n");
		exit(1);
	}

/*This tells the user what inputs were incorrect, if any*/
	if(Iflag != 1 || (Oflag != 1 && displayFlag != 1) || Cflag != 1)
	{
		fprintf(stderr, "The following required parameters were not provided:\n");
		fprintf(stderr, "\t%s%s%s\n", (Iflag==0)?"Frame Directory  ":"",
			(Oflag==0 && displayFlag==0)?"Output File or Display to Screen  ":"",
			(Cflag==0)?"Channel Name  ":"");
		exit(1);
	}

/*This initializes the frame library debug mode*/
	if(debugLevel >= 0)
	{
		int comp = strcmp(debugPath, "stdout");
		if(comp == 0) debugPath = NULL;
		debug = FrLibIni(debugPath, NULL, debugLevel);
		if(debug == NULL || (debug == stdout && comp != 0))
		{
			fprintf(stderr, "Debug file (%s) could not be opened\n", debugPath);
			exit(1);
		}
		if(debugFlag)
			free(debugPath);
	}

/*This gets the list of frame files to read from*/
#ifdef _PC
	sprintf(curFile, "dir %s /B /O:D > %s", framePath, tmpName);
#else
	sprintf(curFile, "ls %s > %s", framePath, tmpName);
#endif
	if(system(curFile))
	{
		fprintf(stderr, "There was an error while getting a directory listing\n");
		exit(1);
	}

	list = fopen(tmpName, "r");
	if(list == NULL)
	{
		fprintf(stderr, "Unable to create and open list of frame files\n");
		exit(1);
	}

/*This takes care of skipping files*/
	if(skipFiles > 0)
	{
		if(ASCIIflag == 0)
			printf("\nSkipping the first %d files\n", skipFiles);
		for(i = 0; i<skipFiles; i++)
		{
			if(fscanf(list, "%s\n", curFile) == EOF)
			{
				fprintf(stderr, "Wanted to skip %d files, but only %d files provided\n", skipFiles, i);
				fclose(list);remove(tmpName);
				exit(1);
			}
			if(ASCIIflag == 0)
				printf("%d:%s, skipped\n", i+1, curFile);
		}
		if(ASCIIflag == 0)
			printf("%d files successfully skipped\n", i);
	}

/*Open the output file*/
	if(displayFlag == 0)
	{
		if((outFile = fopen(outPath, "wb")) == NULL)
		{
			fprintf(stderr, "Unable to open output file. %s\n", outPath);
			fclose(list);remove(tmpName);
			exit(1);
		}
		strcpy(outPath2, outPath);
		strcat(outPath2, ".txt");
		if((outInfo = fopen(outPath2, "w")) == NULL)
		{
			fprintf(stderr, "Unable to open output file. %s\n", outPath2);
			fclose(list);remove(tmpName);
			exit(1);
		}
	}

/*      printf("%s\n", tmpName);*/
/*This loop opens each file*/
	while((fscanf(list, "%s\n", curFile) != EOF) && (pointCount>0))
	{
		int numFrames;
		char* searchPos = NULL;

#ifdef _PC
		if((searchPos = strchr(framePath, '*')) == NULL) {
			searchPos = strchr(framePath, '?'); }
		if(searchPos != NULL)
		{
			while(searchPos != framePath)
			{
				searchPos--;
				if(*searchPos == '\\') break;
			}
			if(searchPos != framePath) searchPos++;
			*searchPos = '\0';
		}
#endif

#ifdef _PC
		if(strchr(curFile, '\\') == NULL && strchr(framePath, '*') == NULL &&
		   strchr(framePath, '?') == NULL &&
		   strcmp(framePath, curFile) != 0)
#else
		if(strchr(curFile, '/') == NULL && strchr(framePath, '*') == NULL &&
		   strchr(framePath, '?') == NULL && strchr(framePath, '[') == NULL &&
		   strcmp(framePath, curFile) != 0)
#endif
		{
			char* tmp;
			int len = 0;
			len = strlen(curFile);
			tmp = malloc(len+1);
			strcpy(tmp, curFile);
			strcpy(curFile, framePath);
			len = strlen(framePath);
#ifdef _PC
			if(*(framePath + (len-1)) != '\\')
				strcat(curFile, "\\");
#else
			if(*(framePath + (len-1)) != '/')
				strcat(curFile, "/");
#endif
			strcat(curFile, tmp);
			free(tmp);
		}

		fileNum++;
		if(ASCIIflag == 0)
			printf("\nOpening file number %d: %s\n", fileNum, curFile);
		iFile = FrFileINew(curFile);
		if(iFile == NULL)
		{
			fprintf(stderr, "Error opening frame file %s\n %s", curFile, FrErrorGetHistory());
			if(displayFlag == 0)
			{
				fprintf(outInfo, "Error opening frame file %s\n %s", curFile, FrErrorGetHistory());
				fprintf(outInfo, "Abnormal Termination\n");
				fclose(outFile);
				fclose(outInfo);
			}
			fclose(list);remove(tmpName);
			exit(1);
		}
		frameNum = 0;

/*This loop reads the specified channel from each frame from the open file*/
		while(((frame = FrameRead(iFile)) != NULL) && (pointCount>0))
		{
			int nWritten;
			frameNum++;

/*Marks the beginning of this frame in seconds*/
			timeStamp = (double)(frame->GTimeS)+1.e-9*(double)(frame->GTimeN);
/*We also need the sampleing rate, but we only need to get it once
  We don't want a time gap error if there is no previous frame*/
			if(fileNum == 1 && frameNum == 1)
			{
/*Find a suitable structure from which to get sampling rate*/
					adc = FrAdcDataFind(frame, channel);
					proc = FrProcDataFind(frame, channel);
					sim = FrSimDataFind(frame, channel);
				if(adc == NULL && proc == NULL && sim == NULL)
				{
					fprintf(stderr, "Error finding appropriate data structure for channel %s\n %s", channel, FrErrorGetHistory());
					fprintf(stderr, "Check the channel name\n");
					if(displayFlag == 0)
					{
						fprintf(outInfo, "Error finding appropriate data structure for channel %s\n %s", channel, FrErrorGetHistory());
						fprintf(outInfo, "Abnormal Termination\n");
						fclose(outFile);
						fclose(outInfo);
					}
					fclose(list);remove(tmpName);
					FrameFree(frame);
					exit(1);
				}
				else
				{
					if(adc != NULL)
						srate = adc->sampleRate;
					if(proc != NULL && proc->data->dx[0] != 0.0)
						srate = 1.0 / proc->data->dx[0];
					if(sim != NULL)
						srate = sim->sampleRate;
					if(srate <= 0)
					{
						fprintf(stderr, "The sampling rate is not positive\n");
						if(displayFlag == 0)
						{
							fprintf(outInfo, "The sampling rate is not positive\n");
							fprintf(outInfo, "Abnormal Termination\n");
							fclose(outFile);
							fclose(outInfo);
						}
						fclose(list);remove(tmpName);
						FrameFree(frame);
						exit(1);
					}
/*Now that we know the sampling rate, we can convert the time to samples if necessary*/
					if(Tflag == 1)
					{
						nPoints = curTime * srate;
						pointCount = nPoints;
					}
					if(skipTime > 0)
					{
						skipPoints = skipTime * srate;
						sPointCount = skipPoints;
					}
				}
/*Writes some info to the log file and initializes the timestamps*/
				if(displayFlag == 0)
				{
					fprintf(outInfo, "The first frame file was %s\n", curFile);
					fprintf(outInfo, "The first frame begins at GPS time %f\n", timeStamp);
					fprintf(outInfo, "%d points (%.3f s) were skipped\n", skipPoints, (double)skipPoints/srate);
					if(AllFlag == 0) {
						fprintf(outInfo, "%d points (%.3f s) were", nPoints, (double)nPoints/srate); }
					else {
						fprintf(outInfo, "all availible data was"); }
					fprintf(outInfo, " written to file\n");
				}
				else {
					fprintf(stderr, "The first frame file is %s\n", curFile); }
				if(ASCIIflag == 0)
					printf("The first frame begins at GPS time %f\n", timeStamp);
				prevStamp = timeStamp;
			}

/*If the difference in time between the end of the last frame and the beginning
  of current one is greater than the resolution (defined at the beginning), then
  the frames are nonsequencial*/
			if(fabs(timeStamp-prevStamp)>RESOLUTION)
			{
				double t = timeStamp-prevStamp;
				int len, prevFrame = (frameNum-1>0)?frameNum-1:numFrames;
				int prevFile = (frameNum-1>0)?fileNum:fileNum-1;
				gapCount++;
				fprintf(stderr, "\n********************************************************************************\n");
				fprintf(stderr, "                          !!!FRAMES NOT SEQUENCIAL!!!\n");
				for(len=0;len<(80-((int)log10(t)+1+4+40))/2;len++)
					fprintf(stderr, " ");
				fprintf(stderr, "There is a time gap of %.3f seconds between:\n", t);
				if(prevFrame == 0) prevFrame = -1;
				if(frameNum == 0) frameNum = -1;
				for(len=0;len<(80-((int)log10(abs(prevFrame*prevFile*frameNum*fileNum))+4+33))/2;len++)
					fprintf(stderr, " ");
				if(prevFrame == -1) prevFrame = 0;
				if(frameNum == -1) frameNum = 0;
				fprintf(stderr, "Frame %d of File %d & Frame %d of File %d\n", prevFrame, prevFile, frameNum, fileNum);
				if(zeroFlag != 1) {
					fprintf(stderr, " Output will continue as normal, use -z option to output zeros for missing time\n"); }

/*This is the implematation of the -z option.  It outputs zeros for the number
  of seconds of any gap between two frames*/
				else
				{
					int NP = t*srate, j;
					void *Zarray;
					Zarray = calloc(NP, wSize);
					if(Zarray != NULL)
					{
						for(len=0;len<(80-((int)log10(t)+1+4+28))/2;len++)
							fprintf(stderr, " ");
						fprintf(stderr, "Outputting %.3f seconds of zeros\n", t);
						nWritten = fwrite(Zarray, wSize, NP, outFile);
						for(len=0;len<(80-((int)log10(nWritten)+1+26))/2;len++)
							fprintf(stderr, " ");
						fprintf(stderr, "%d zeros sucessfully written\n", nWritten);
					}
					else {
						fprintf(stderr, "                   Not enough memory to initialize zero array\n"); }
					free(Zarray);
				}
				fprintf(stderr, "********************************************************************************\n\n");
			}

/*Sets prevStamp to the time at the end of current frame*/
			prevStamp = timeStamp+frame->dt;

/*Reads the data vector from the frame file*/
			if(ASCIIflag == 0)
				printf("Reading channel %s from frame %d of file %d\n", channel, frameNum, fileNum);

			frvect = FrameFindVect(frame, channel);
			if(frvect == NULL)
			{
				fprintf(stderr, "Channel '%s' was not found in file %s\n %s", channel, curFile, FrErrorGetHistory());
				if(displayFlag == 0)
				{
					fprintf(outInfo, "Channel '%s' was not found in file %s\n %s", channel, curFile, FrErrorGetHistory());
					fprintf(outInfo, "Abnormal Termination\n");
					fclose(outFile);
					fclose(outInfo);
				}
				fclose(list);remove(tmpName);
				FrameFree(frame);
				FrFileIEnd(iFile);
				exit(1);
			}

			wSize = frvect->wSize;
			nData = frvect->nData;
			type  = frvect->type;

/*Skip points if there are points to be skipped*/
			if(sPointCount >= nData)
			{
				sPointCount -= nData;
				if(ASCIIflag == 0)
					printf("%d samples (%.3f s) skipped\n", nData, (double)nData/srate);
				continue;
			}

/*If there are more points to write than there are in the frame, then write the
  entire frame, otherwise write whatever is left*/
			if(displayFlag == 0)
			{
				if(pointCount > nData - sPointCount)
				{
					if(intFlag == 1 && type != FR_VECT_2S) {
						nWritten = writeShortInt(type, frvect->data, nData - sPointCount, sPointCount*wSize, outFile); }
					else if(floatFlag == 1 && type != FR_VECT_4R) {
						nWritten = writeFloat(type, frvect->data, nData - sPointCount, sPointCount*wSize, outFile); }
					else {
						nWritten = fwrite(frvect->data + sPointCount*wSize, wSize, nData - sPointCount, outFile); }
				}
				else
				{
					if(intFlag == 1 && type != FR_VECT_2S) {
						nWritten = writeShortInt(type, frvect->data, pointCount - sPointCount, sPointCount*wSize, outFile); }
					else if(floatFlag == 1 && type != FR_VECT_4R) {
						nWritten = writeFloat(type, frvect->data, pointCount - sPointCount, sPointCount*wSize, outFile); }
					else {
						nWritten = fwrite(frvect->data + sPointCount*wSize, wSize, pointCount - sPointCount, outFile); }
				}
				if(nWritten == -1)
				{
					if(displayFlag == 0)
					{
						fprintf(outInfo, "Could not allocate memory to convert data type", channel, curFile, FrErrorGetHistory());
						fprintf(outInfo, "Abnormal Termination\n");
						fclose(outFile);
						fclose(outInfo);
					}
					fclose(list);remove(tmpName);
					FrameFree(frame);
					FrFileIEnd(iFile);
					exit(1);
				}
			}

/*This displays the samples to the screen is the option was provided*/
			else if((displayFlag == 1) && isValidType(type))
			{
				const int numPerRow = 7;
				void *data;
				char keyIn;
				int disCount = sPointCount, j;
				data = frvect->data;
				while(disCount + numPerRow < nData)
				{
					if(ASCIIflag == 0)
					{
						printf("%08d: ", nPoints - pointCount + skipPoints + 1);
						for(j = 0; j<numPerRow; j++)
							printSample(type, data, disCount + j);
					}
					else
					{
						for(j = 0; j<numPerRow; j++)
						{
							printUnformattedSample(type, data, disCount + j);
							printf("\n");
						}
					}
					if(ASCIIflag == 0)
						printf("\n");
					disCount += numPerRow;
					pointCount -= numPerRow;
					if(pointCount <= 0)
					{
						pointCount = 0;
						break;
					}

/*Pause after the screen is full*/
					if(disCount%(23*numPerRow) == 0 && ASCIIflag == 0)
					{
						printf("Press Enter to continue or 'x' to exit: ");
						keyIn = fgetc(stdin);
						if(keyIn == 'x' || keyIn == 'X')
						{
							printf("User break\n");
							fclose(list);remove(tmpName);
							FrameFree(frame);
							FrFileIEnd(iFile);
							exit(0);
						}
					}
				}
				if(pointCount == 0)
				{
					sPointCount = 0;
					continue;
				}
				if(ASCIIflag == 0)
				{
					printf("%08d: ", nPoints - pointCount + skipPoints + 1);
					for(j = 0; j<nData-disCount; j++)
						printSample(type, data, disCount + j);
				}
				else
				{
					for(j = 0; j<nData-disCount; j++)
					{
						printUnformattedSample(type, data, disCount + j);
						printf("\n");
					}
				}
				if(ASCIIflag == 0)
					printf("\n");
				pointCount -= nData-disCount;

/*Pause after each frame as well*/
				if(ASCIIflag == 0)
				{
					printf("Press Enter to continue or 'x' to exit: ");
					keyIn = fgetc(stdin);
					if(keyIn == 'x' || keyIn == 'X')
					{
						printf("User break\n");
						fclose(list);remove(tmpName);
						FrameFree(frame);
						FrFileIEnd(iFile);
						exit(0);
					}
				}
			}
			else
			{
				fprintf(stderr, "\nunknown data type: cannot display\n");
				fclose(list);remove(tmpName);
				/*fclose(outFile);
				fclose(outInfo);*/
				FrameFree(frame);
				FrFileIEnd(iFile);
				exit(1);
			}


			if(sPointCount > 0)
			{
				if(displayFlag == 0)
					printf("%d samples (%.3f s) skipped\n", sPointCount, (double)sPointCount/srate);
				sPointCount = 0;
			}
			if(displayFlag == 0)
			{
				printf("%d samples (%.3f s) successfully written\n", nWritten, (double)nWritten/srate);
				pointCount -= nWritten;
			}
/*Free the memory for the frame structure*/
			FrameFree(frame);
		}
		if(displayFlag == 0)
		{
			int read = nPoints - pointCount + skipPoints - sPointCount;
			int written = nPoints - pointCount;
			printf("%d total samples (%.3f s) read and %d (%.3f s) written thus far\n",
				read, (double)read/srate, written, (double)written/srate);
		}
		if(frameNum == 0)
		{
			fprintf(stderr, "No frames were found in the file %s\n", curFile);
			fprintf(stderr, "!!!There may be something wrong with the file!!!\n");
			if(displayFlag == 0)
			{
				fprintf(outInfo, "No frames were found in the file %s\n", curFile);
				fprintf(outInfo, "!!!There may be something wrong with the file!!!\n");
			}
		}
		numFrames = frameNum;

/*Close the frame file*/
		FrFileIEnd(iFile);
	}
	if((pointCount != 0 || sPointCount != 0) && (AllFlag == 0))
	{
		int req = nPoints + skipPoints;
		int avail = nPoints - pointCount + skipPoints - sPointCount;
		fprintf(stderr, "\n%d samples (%.3f s) were requested and only %d samples (%.3f s) were availible\n",
			req, (double)req/srate, avail, (double)avail/srate);
		if(displayFlag == 0)
			fprintf(outInfo, "\n%d samples (%.3f s) were requested and only %d samples (%.3f s) were availible\n",
				req, (double)req/srate, avail, (double)avail/srate);
	}

	if(nPoints - pointCount > 0)
	{
		int read = nPoints - pointCount + skipPoints - sPointCount;
		int written = nPoints - pointCount;
		if(displayFlag == 0)
		{
			fprintf(outInfo, "\n%d files opened\n", fileNum);
			fprintf(outInfo, "%d total samples (%.3f s) read and %d (%.3f s) written\n",
					read, (double)read/srate, written, (double)written/srate);
		}
		else
		{
			fprintf(stderr, "\n%d files opened\n", fileNum);
			fprintf(stderr, "%d total samples (%.3f s) read and %d (%.3f s) written to standard out\n",
					read, (double)read/srate, written, (double)written/srate);
		}

		if(intFlag == 1)
		{
			type = FR_VECT_2S;
			wSize = sizeof(short);
		}
		else if(floatFlag == 1)
		{
			type = FR_VECT_4R;
			wSize = sizeof(float);
		}

		getDataType(type, typeStr);
		if(displayFlag == 0)
		{
			printf("\nThe binary output file (%s) is of type '%s'\n", outPath, typeStr);
			printf("The word length is %d byte%c\n", wSize, (wSize==1)?'\0':'s');
			printf("The sampling rate is %f sample%c/sec\n", srate, (srate==1)?'\0':'s');
			fprintf(outInfo, "\nThe binary output file (%s) is of type '%s'\n", outPath, typeStr);
			fprintf(outInfo, "The word length is %d byte%c\n", wSize, (wSize==1)?'\0':'s');
			fprintf(outInfo, "The sampling rate is %f sample%c/sec\n", srate, (srate==1)?'\0':'s');
		}
		else {
			fprintf(stderr, "The sampling rate is %f sample%c/sec\n", srate, (srate==1)?'\0':'s'); }
		fprintf(stderr, "The data output was %s", (gapCount==0)?"continuous":"not continuous");
		if(displayFlag == 0)
			fprintf(outInfo, "The data output was %s", (gapCount==0)?"continuous":"not continuous");
		if(gapCount>0)
		{
			fprintf(stderr, "; there %s %d gap%c between frames\n\n", (gapCount==1)?"is":"are", gapCount,
				(gapCount==1)?'\0':'s');
			if(displayFlag == 0)
				fprintf(outInfo, "; there %s %d gap%c between frames\n\n", (gapCount==1)?"is":"are", gapCount,
					(gapCount==1)?'\0':'s');
		}
		else {
			fprintf(stderr, "\n"); }
	}
	fclose(list);remove(tmpName);
	if(displayFlag == 0)
	{
		fclose(outFile);
		fclose(outInfo);
	}
}

void printSample(int type, void *data, int i)
{
	switch(type)
	{
		case FR_VECT_C:
			printf("%9hhd ", ((char*)data)[i]);break;
		case FR_VECT_2S:
			printf("%9hd ", ((short*)data)[i]);break;
		case FR_VECT_4S:
			printf("%9d ", ((int*)data)[i]);break;
		case FR_VECT_8S:
			printf("%9ld ", ((long*)data)[i]);break;
		case FR_VECT_1U:
			printf("%9c ", ((unsigned char*)data)[i]);break;
		case FR_VECT_2U:
			printf("%9hu ", ((unsigned short*)data)[i]);break;
		case FR_VECT_4U:
			printf("%9u ", ((unsigned int*)data)[i]);break;
		case FR_VECT_8U:
			printf("%9lu ", ((unsigned long*)data)[i]);break;
		case FR_VECT_4R:
			printf("%9e ", ((float*)data)[i]);break;
		case FR_VECT_8R:
			printf("%9e ", ((double*)data)[i]);break;
	}
}

void printUnformattedSample(int type, void *data, int i)
{
	switch(type)
	{
		case FR_VECT_C:
			printf("%hhd", ((char*)data)[i]);break;
		case FR_VECT_2S:
			printf("%hd", ((short*)data)[i]);break;
		case FR_VECT_4S:
			printf("%d", ((int*)data)[i]);break;
		case FR_VECT_8S:
			printf("%ld", ((long*)data)[i]);break;
		case FR_VECT_1U:
			printf("%c", ((unsigned char*)data)[i]);break;
		case FR_VECT_2U:
			printf("%hu", ((unsigned short*)data)[i]);break;
		case FR_VECT_4U:
			printf("%u", ((unsigned int*)data)[i]);break;
		case FR_VECT_8U:
			printf("%lu", ((unsigned long*)data)[i]);break;
		case FR_VECT_4R:
			printf("%9e ", ((float*)data)[i]);break;
		case FR_VECT_8R:
			printf("%9e ", ((double*)data)[i]);break;
	}
}

int writeShortInt(int type, char *data, int length, int offset, FILE *outFile)
{
	short *dataS;
	int nWritten = 0, i;
	if((dataS = malloc(length*sizeof(short))) == NULL)
	{
		printf("Could not allocate memory to convert data to short ints\n");
		return -1;
	}
	switch(type)
	{
		case FR_VECT_C:
		{
			char *dataC = data + offset;
			for(i = 0; i < length; i++)
				dataS[i] = (short)dataC[i];
			nWritten = fwrite(dataS, sizeof(short), length, outFile);
			break;
		}
		case FR_VECT_4S:
		{
			int *dataI = (int*)(data + offset);
			for(i = 0; i < length; i++)
				dataS[i] = (short)dataI[i];
			nWritten = fwrite(dataS, sizeof(short), length, outFile);
			break;
		}
		case FR_VECT_8S:
		{
			long *dataL = (long*)(data + offset);
			for(i = 0; i < length; i++)
				dataS[i] = (short)dataL[i];
			nWritten = fwrite(dataS, sizeof(short), length, outFile);
			break;
		}
		case FR_VECT_1U:
		{
			unsigned char *dataC = (unsigned char*)(data + offset);
			for(i = 0; i < length; i++)
				dataS[i] = (short)dataC[i];
			nWritten = fwrite(dataS, sizeof(short), length, outFile);
			break;
		}
		case FR_VECT_2U:
		{
			unsigned short *dataC = (unsigned short*)(data + offset);
			for(i = 0; i < length; i++)
				dataS[i] = (short)dataC[i];
			nWritten = fwrite(dataS, sizeof(short), length, outFile);
			break;
		}
		case FR_VECT_4U:
		{
			unsigned int *dataI = (unsigned int*)(data + offset);
			for(i = 0; i < length; i++)
				dataS[i] = (short)dataI[i];
			nWritten = fwrite(dataS, sizeof(short), length, outFile);
			break;
		}
		case FR_VECT_8U:
		{
			unsigned long *dataL = (unsigned long*)(data + offset);
			for(i = 0; i < length; i++)
				dataS[i] = (short)dataL[i];
			nWritten = fwrite(dataS, sizeof(short), length, outFile);
			break;
		}
		case FR_VECT_4R:
		{
			float *dataF = (float*)(data + offset);
			for(i = 0; i < length; i++)
				dataS[i] = (short)dataF[i];
			nWritten = fwrite(dataS, sizeof(short), length, outFile);
			break;
		}
		case FR_VECT_8R:
		{
			double *dataD = (double*)(data + offset);
			for(i = 0; i < length; i++)
				dataS[i] = (short)dataD[i];
			nWritten = fwrite(dataS, sizeof(short), length, outFile);
			break;
		}
	}
	free(dataS);
	return nWritten;
}

int writeFloat(int type, char *data, int length, int offset, FILE *outFile)
{
	float *dataF;
	int nWritten = 0, i;
	if((dataF = malloc(length*sizeof(float))) == NULL)
	{
		printf("Could not allocate memory to convert data to floats\n");
		return -1;
	}
	switch(type)
	{
		case FR_VECT_C:
		{
			char *dataC = data + offset;
			for(i = 0; i < length; i++)
				dataF[i] = (float)dataC[i];
			nWritten = fwrite(dataF, sizeof(float), length, outFile);
			break;
		}
		case FR_VECT_4S:
		{
			int *dataI = (int*)(data + offset);
			for(i = 0; i < length; i++)
				dataF[i] = (float)dataI[i];
			nWritten = fwrite(dataF, sizeof(float), length, outFile);
			break;
		}
		case FR_VECT_8S:
		{
			long *dataL = (long*)(data + offset);
			for(i = 0; i < length; i++)
				dataF[i] = (float)dataL[i];
			nWritten = fwrite(dataF, sizeof(float), length, outFile);
			break;
		}
		case FR_VECT_1U:
		{
			unsigned char *dataC = (unsigned char*)(data + offset);
			for(i = 0; i < length; i++)
				dataF[i] = (float)dataC[i];
			nWritten = fwrite(dataF, sizeof(float), length, outFile);
			break;
		}
		case FR_VECT_2U:
		{
			unsigned short *dataS = (unsigned short*)(data + offset);
			for(i = 0; i < length; i++)
				dataF[i] = (float)dataS[i];
			nWritten = fwrite(dataF, sizeof(float), length, outFile);
			break;
		}
		case FR_VECT_4U:
		{
			unsigned int *dataI = (unsigned int*)(data + offset);
			for(i = 0; i < length; i++)
				dataF[i] = (float)dataI[i];
			nWritten = fwrite(dataF, sizeof(float), length, outFile);
			break;
		}
		case FR_VECT_8U:
		{
			unsigned long *dataL = (unsigned long*)(data + offset);
			for(i = 0; i < length; i++)
				dataF[i] = (float)dataL[i];
			nWritten = fwrite(dataF, sizeof(float), length, outFile);
			break;
		}
		case FR_VECT_2S:
		{
			short *dataS = (short*)(data + offset);
			for(i = 0; i < length; i++)
				dataF[i] = (float)dataS[i];
			nWritten = fwrite(dataF, sizeof(float), length, outFile);
			break;
		}
		case FR_VECT_8R:
		{
			double *dataD = (double*)(data + offset);
			for(i = 0; i < length; i++)
				dataF[i] = (float)dataD[i];
			nWritten = fwrite(dataF, sizeof(float), length, outFile);
			break;
		}
	}
	free(dataF);
	return nWritten;
}

void getDataType(int type, char *typeStr)
{
	switch(type)
	{
		case FR_VECT_C:
			strcpy(typeStr, "char");break;
		case FR_VECT_2S:
			strcpy(typeStr, "short");break;
		case FR_VECT_4S:
			strcpy(typeStr, "int");break;
		case FR_VECT_8S:
			strcpy(typeStr, "long");break;
		case FR_VECT_1U:
			strcpy(typeStr, "unsigned char");break;
		case FR_VECT_2U:
			strcpy(typeStr, "unsigned short");break;
		case FR_VECT_4U:
			strcpy(typeStr, "unsigned int");break;
		case FR_VECT_8U:
			strcpy(typeStr, "unsigned long");break;
		case FR_VECT_4R:
			strcpy(typeStr, "float");break;
		case FR_VECT_8R:
			strcpy(typeStr, "double");break;
		default:
			strcpy(typeStr, "unknown");break;
	}
}

int isValidType(int type)
{
	if(type == FR_VECT_4R || type == FR_VECT_8R || type == FR_VECT_2S ||
	   type == FR_VECT_4S || type == FR_VECT_8S || type == FR_VECT_1U ||
	   type == FR_VECT_2U || type == FR_VECT_4U || type == FR_VECT_8U ||
	   type == FR_VECT_C) {
			return 1; }
	else {
		return 0; }
}
