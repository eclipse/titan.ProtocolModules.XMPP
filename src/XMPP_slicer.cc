/******************************************************************************
* Copyright (c) 2000-2018 Ericsson Telecom AB
*
* All rights reserved. This program and the accompanying materials
* are made available under the terms of the Eclipse Public License v1.0
* which accompanies this distribution, and is available at
* http://www.eclipse.org/legal/epl-v10.html
*
* Contributors:
*   Gabor Szalai - initial implementation
******************************************************************************/
//This function permits extracting XMPP messages from a TCP stream by calculating the message length
//
//  File:          XMPP_slicer.cc
//  Description:
//  References:
//  Rev:           R3A
//  Prodnr:        CNL 113 775
//
////////////////////////////////////////////////////////////////////////////////


#include "XMPP_common.hh"
#include <ctype.h>
#include <string.h>

namespace XMPP__common {

INTEGER f__calc__XML__length(const OCTETSTRING& data, const OCTETSTRING& stream__header__name){
  int data_length=data.lengthof();
  const unsigned char* ptr=(const unsigned char*)data;
  int state=0;
  int tag_index=0;
  int tag_length=0;
  const unsigned char* tag_ptr=NULL;
  int closing_char_num=0;

  //int stream_index=0;
  int stream_length=stream__header__name.lengthof();
  const unsigned char* stream_ptr=(const unsigned char*)stream__header__name;

//printf("Start\r\n");  
  for(int i=0; i<data_length; i++){
//printf("i=%d state=%d ptr[i]%c tag_index=%d tag_length=%d\r\n",i,state,ptr[i],tag_index,tag_length);
    switch(state){
      case 0:                // initial
        if(ptr[i]=='<'){
          state=1;
        }
        break;
      case 1:                //  < found
        switch(ptr[i]){
          case '?':          //  <? found, look for the next <
            state=0;
            break;
          case '!':          //  <!  possible comment and CDATA
            state=2;
            break;
          case '/':          //  </ closing tag: compare tag name
            if(!tag_ptr){
              state=9;       // no tag name stored, must be the stream close tag, or something went wrong
            } else {
              state=3;       // tag name stored, look for the tag name
            }
            tag_index=0;
            break;
          case ' ':          // white spaces, skip
          case '\t':
          case '\r':
          case '\n':
            break;
          default:           // tag name, store it if needed
            if(!tag_ptr){
              tag_ptr=ptr+i;
              tag_length++;   // tag name char
              state=4;
            } else {
              state=0;       // tag name already stored. look for the next tag
            }
        }
        break;
      case 2: //  <!  possible comment and CDATA
        switch(ptr[i]){
          case '-':          // <!-  comment
            state=5;
            closing_char_num=0;
            break;
          case '[':          // <![  CDATA
            closing_char_num=0;
            state=6;
            break;
          default:           // something else, look for the next tag
            state=0;
        }
        break;
      case 3:           //  </ closing tag: compare tag name
        switch(ptr[i]){
          case ' ':          // white spaces or > end of tag
          case '\t':
          case '\r':
          case '\n':
          case '>':
            if(tag_index==tag_length){  // tag found
              if(ptr[i]=='>'){   // final > found
                return i+1;      // length = index + 1
              } else {
                state=7;         // look for final >
              }
            }
            break;
          default:
            if(tag_ptr && tag_index<tag_length && tolower(tag_ptr[tag_index])==tolower(ptr[i])){
              tag_index++;       // mathcing char
            } else {             
              tag_index=0;       // no match, next tag
              state=0;
            }
        }
        break;
      case 4:   // tag name, store it
        switch(ptr[i]){
          case ' ':          // white spaces or > end of tag name
          case '\t':
          case '\r':
          case '\n':
            if(stream_length==tag_length && !strncmp((const char*)tag_ptr,(const char*)stream_ptr,stream_length)){       // tag name is the stream tag
              state=9;  // look for the end of the stream tag
            } else {
              state=8;  // look for the end of the current tag
            }       
            closing_char_num=0;
            break;
          case '/':
            if(stream_length==tag_length && !strncmp((const char*)tag_ptr,(const char*)stream_ptr,stream_length)){       // tag name is the stream tag
              state=9;  // look for the end of the stream tag
            } else {
              state=8;  // look for the end of the current tag
            }       
            closing_char_num=1;
            break;
          case '>':
            if(stream_length==tag_length && !strncmp((const char*)tag_ptr,(const char*)stream_ptr,stream_length)){       // tag name is the stream tag
              return i+1;
            }
            state=0;        // end of tag, look for the next one
            break;
          default:
            tag_length++;   // tag name char
            break;
        }
        break;
      case 5:  // comment, ends with -->
        switch(ptr[i]){
          case '-':
            closing_char_num++;
            break;
          case '>':
            if(closing_char_num>=2){  // --> found
              state=0;          // end of comment look for next tag
            } else {
              closing_char_num=0;
            }
            break;
          default:
            closing_char_num=0;
            break;
        }
        break;
      case 6:  // CDATA, ends with ]]>
        switch(ptr[i]){
          case ']':
            closing_char_num++;
            break;
          case '>':
            if(closing_char_num>=2){  // --> found
              state=0;          // end of CDATA look for next tag
            } else {
              closing_char_num=0;
            }
            break;
          default:
            closing_char_num=0;
            break;
        }
        break;

      case 7: // look for final >
        if(ptr[i]=='>'){
          return i+1; // length = index + 1
        }
        break;

      case 8: // check the end of the openning tag, if the opening tag ends with /> we found the end
        switch(ptr[i]){
          case '/':
            closing_char_num=1;
            break;
          case '>':
            if(closing_char_num==1){  // /> found
              return  i+1; // length = index + 1
            } else {
              state=0;   // not />. look for the next tag
            }
            break;
          default:
            closing_char_num=0;
            break;
        }
        break;
        
      case 9:   // stream tag end
          if(ptr[i]=='>'){
            return  i+1; // length = index + 1
          }
        break;
        
      default:
        break;
    }
  }

  return -1;
}

}

