/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// "liveMedia"
// Copyright (c) 1996-2014 Live Networks, Inc.  All rights reserved.
// A source that consists of multiple byte-stream files, read sequentially
// Implementation

#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include "ByteStreamMultiFileSource.hh"

char *snap_store_path;

//#define DEBUG

//
//// definations for get file name list
//
#define	TIME_SPAN 600
#define	FN_LIST_SIZE 100
#define	NAMLEN(dirent) strlen((dirent)->d_name)

/* compare function for	qsort()	*/
static int fn_cmp (const void *c1, const void *c2)
{
	return strcmp (((struct	snap_fn	*)c1)->fn, ((struct snap_fn *)c2)->fn);
}

/* get filename	list */
static int fc(char *path, long	st_i, long et_i, struct	snap_fn	*list)
{
	DIR *dp,*dp2;
	struct dirent *ep, *ep2;
	char tmp[100];
	int total=0, st_d, et_d, this_d;
	long this_i;

	st_d = st_i/TIME_SPAN;
	et_d = et_i/TIME_SPAN;
  
	dp = opendir(path);
	if (dp != NULL){
		while ((ep = readdir(dp))){
			if (ep->d_name[0]=='.' && 
				(NAMLEN(ep)==1 ||(ep->d_name[1]=='.' &&	NAMLEN(ep)==2))) continue;
			this_d=atoi(ep->d_name);
			if (this_d<st_d	|| this_d>et_d)	continue;
			sprintf(tmp, "%s/%s", path, ep->d_name);
			dp2 = opendir (tmp);
			if (dp2	!= NULL){
				while ((ep2 = readdir(dp2))){
					if (total==FN_LIST_SIZE) break;
					if (ep2->d_name[0]=='.'	&& 
						(NAMLEN(ep2)==1	||
							(ep2->d_name[1]=='.' &&	NAMLEN(ep2)==2))) continue;
					this_i=atol(ep2->d_name);
					if (this_i<st_i	|| this_i>et_i)	continue;
					strcpy(list[total].fn, ep2->d_name);
					strcpy(list[total].dn, ep->d_name);
					/* printf("%u: %s/%s\n", childs, list[total].dn, list[total].fn); */
					total++;
				}
				(void) closedir	(dp2);
			}
		}
		(void) closedir	(dp);
	}
  
  return total;
}

int ByteStreamMultiFileSource::getNewFileList()
{
    long st_i, et_i;
    char tmp_str[50], cam_path[200];

    
    if (fn_list==NULL) return -1;

#ifdef DEBUG
    fprintf(stderr, "getNewFileList(): lastTick = %ld\n", lastTick);
#endif

    /* find filenames */
    st_i = lastTick;
    et_i = st_i+300; /* 5 mins */
    sprintf(cam_path, "%s/%s", snap_store_path, camid);	
    fNumSources=fc(cam_path, st_i, et_i, fn_list);
    printf("total %u clips: \n", fNumSources);
    if (fNumSources==0) return -1;
    	
    qsort(fn_list, fNumSources, sizeof(struct snap_fn), fn_cmp);

    // Next, copy the source file names into our own array:
    fFileNameArray = new char const*[fNumSources];
    if (fFileNameArray == NULL) return -1;
    unsigned i;
    for (i = 0; i < fNumSources; ++i) {
      sprintf(tmp_str, "%s/%s", fn_list[i].dn, fn_list[i].fn);
      fFileNameArray[i] = strDup(tmp_str);
    }
    lastTick=atol(fn_list[fNumSources-1].fn)+1;
    //fprintf(stderr, "getNewFileList(): new lastTick = %ld\n", lastTick);

    // Next, set up our array of component ByteStreamFileSources
    // Don't actually create these yet; instead, do this on demand
    fSourceArray = new ByteStreamFileSource*[fNumSources];
    if (fSourceArray == NULL) return -1;
    for (i = 0; i < fNumSources; ++i) {
      fSourceArray[i] = NULL;
    }
    
    return 0;
}

ByteStreamMultiFileSource
::ByteStreamMultiFileSource(UsageEnvironment& env, char const *fileName, 
			    unsigned preferredFrameSize, unsigned playTimePerFrame)
  : FramedSource(env),
    fPreferredFrameSize(preferredFrameSize), fPlayTimePerFrame(playTimePerFrame),
    fCurrentlyReadSourceNumber(0), fHaveStartedNewFile(False) 
{
	char *p, *lt;
	fSourceArray=NULL;
	fFileNameArray=NULL;
	fNumSources=0;

	do{
		/* get camid */
		camid=strDup(fileName);
		
		p=camid;
		for (; *p&&*p!='_'; p++);
		if (!*p){
			printf("ByteStreamMultiFileSource(): parse <camid> fail.\n");
			break;
		}
		*p='\0';

		/* get start tick */
		lt=++p;
		for (; *p&&*p!='_'; p++);
		if (!*p){
			printf("ByteStreamMultiFileSource(): parse <start tick> fail.\n");
			break;
		}
		*p='\0';
	
		lastTick=atol(lt);
		
		/* get play speed */
		p++;
		fSpeed=atoi(p);
#ifdef DEBUG		
		printf("ByteStreamMultiFileSource(): %s %ld %u.\n", camid, lastTick, fSpeed);
#endif
	} while(0);
	
	fn_list=new struct snap_fn[FN_LIST_SIZE];
	if (fn_list!=NULL) getNewFileList();
}

void ByteStreamMultiFileSource::myFree()
{
	unsigned i;
  
	if (fSourceArray!=NULL){
		for (i = 0; i < fNumSources; ++i) {
			Medium::close(fSourceArray[i]);
		}
		delete[] fSourceArray;
		fSourceArray=NULL;
	}

	if (fFileNameArray!=NULL){
		for (i = 0; i < fNumSources; ++i) {
			delete[] (char*)(fFileNameArray[i]);
		}
		delete[] fFileNameArray;
		fFileNameArray=NULL;
	}
}

ByteStreamMultiFileSource::~ByteStreamMultiFileSource() {
	myFree();
	delete[] fn_list;
	delete[] (char*)(camid);
}

ByteStreamMultiFileSource* ByteStreamMultiFileSource
::createNew(UsageEnvironment& env, char const *fileName,
	    unsigned preferredFrameSize, unsigned playTimePerFrame) {
  ByteStreamMultiFileSource* newSource
    = new ByteStreamMultiFileSource(env, fileName, preferredFrameSize, playTimePerFrame);

  return newSource;
}

void ByteStreamMultiFileSource::doGetNextFrame() {
  char fileName[200];
  
  do {
    // First, check whether we've run out of sources:
    if (fCurrentlyReadSourceNumber >= fNumSources){
    	myFree();
    	if (getNewFileList()==-1) break;
    	fCurrentlyReadSourceNumber=0;
    }

    fHaveStartedNewFile = False;
    ByteStreamFileSource*& source
      = fSourceArray[fCurrentlyReadSourceNumber];
    if (source == NULL) {
      // The current source hasn't been created yet.  Do this now:
      sprintf(fileName, "%s/%s/%s", snap_store_path, camid, fFileNameArray[fCurrentlyReadSourceNumber]);
      source = ByteStreamFileSource::createNew(envir(),
		       fileName,
		       fPreferredFrameSize, fPlayTimePerFrame);
      if (source == NULL) break;
      fHaveStartedNewFile = True;
#ifdef DEBUG
      printf("new file source: %s\n", fileName);
#endif
    }

    // (Attempt to) read from the current source.
    source->getNextFrame(fTo, fMaxSize,
			       afterGettingFrame, this,
			       onSourceClosure, this);
    return;
  } while (0);

  // An error occurred; consider ourselves closed:
  handleClosure();
}

void ByteStreamMultiFileSource
  ::afterGettingFrame(void* clientData,
		      unsigned frameSize, unsigned numTruncatedBytes,
		      struct timeval presentationTime,
		      unsigned durationInMicroseconds) {
  ByteStreamMultiFileSource* source
    = (ByteStreamMultiFileSource*)clientData;
  source->fFrameSize = frameSize;
  source->fNumTruncatedBytes = numTruncatedBytes;
  source->fPresentationTime = presentationTime;
  source->fDurationInMicroseconds = durationInMicroseconds;
  FramedSource::afterGetting(source);
}

void ByteStreamMultiFileSource::onSourceClosure(void* clientData) {
  ByteStreamMultiFileSource* source
    = (ByteStreamMultiFileSource*)clientData;
  source->onSourceClosure1();
}

void ByteStreamMultiFileSource::onSourceClosure1() {
  // This routine was called because the currently-read source was closed
  // (probably due to EOF).  Close this source down, and move to the
  // next one:
  ByteStreamFileSource*& source
    = fSourceArray[fCurrentlyReadSourceNumber++];
  Medium::close(source);
  source = NULL;

  // Try reading again:
  doGetNextFrame();
}
