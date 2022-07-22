/*
  Copyright (c) 2022 Makoto Tanabe <mtanabe.sj@outlook.com>
  Licensed under the MIT License.

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/
#pragma once
#include "helper.h"


// this class implements a command argument with or without an optional setting. so, if the base ustring is set to, e.g., a single letter of 'l' signifying a logging option, the member variable _setting may have a path name of a file a log is written to.
class CommandArg : public ustring
{
public:
	CommandArg(LPCWSTR src, int len) : ustring(src, len), _setting(NULL) {}
	~CommandArg()
	{
		if (_setting)
			delete _setting;
	}
	// if defined, it is an optional setting of the command arg.
	ustring *_setting;
};



class CommandArgs : public objList<CommandArg>
{
public:
	CommandArgs() {}

	// this is the first method to call after instantiating. the parsed items are in the objList. use its array indexing operator [] to access the items (command arguments).
	int parse(LPCWSTR lpCmdLine)
	{
		split(lpCmdLine, lstrlen(lpCmdLine));
		return ERROR_SUCCESS;
	}
	// after parse is called, use this method passing a single letter, e.g., 'x', which stands for a optional flag. the method searches for an item whose ustring base object matches the letter. if a match is found, a pointer to a command argument corresponding to the flag is returned.
	CommandArg *getOption(WCHAR flagLetter)
	{
		for (int i = 0; i < _n; i++)
		{
			if ((*this)[i]->_buffer[0] == '-' && (*this)[i]->_buffer[1] == flagLetter)
			{
				return (*this)[i];
			}
		}
		return NULL;
	}

protected:
	// pulls out from an input string, src, a term whose beginning and ending characters are the input targetChar. the search runs from startPos to srcLen. startPos must be in the range of 0 to srcLen-1.
	// split uses _getTerm to extract an optional setting double quoted or not quoted after it finds an item followed with a ':', a prefix for an optional setting.
	template<class T>
	T *_getTerm(WCHAR targetChar, LPCWSTR src, int srcLen, int &startPos)
	{
		T* ex = NULL;
		for (int j = startPos; j < srcLen; j++) {
			// look for a closing space (or quotation mark).
			if (src[j] == targetChar) {
				// get the inside text length.
				int len = j - startPos;
				// return it as a new term.
				if (len > 0)
					ex = new T(src + startPos, len);
				startPos = j; // startPos at the closing mark.
				return ex;
			}
		}
		// the target char has not been found.
		// all chars in the source buffer make up one term.
		if (startPos < srcLen)
			ex = new T(src + startPos, srcLen - startPos);
		startPos = srcLen;
		return ex;
	}
	// splits the input command line (p0 of length len0) into an array of command arguments.
	bool split(LPCWSTR p0, int len0)
	{
		int i, len;
		int i0 = 0;
		for (i = 0; i < len0; i++)
		{
			// watch for double quotes.
			if (p0[i] == '"')
			{
				// got an opening double quotation mark.
				i0 = i + 1;
				CommandArg *x = _getTerm<CommandArg>(L'"', p0, len0, i0);
				if (x)
				{
					objList<CommandArg>::add(x);
					i = i0++;
				}
			}
			// a space char separates terms.
			else if (p0[i] == ' ')
			{
				// here is a new term.
				len = i - i0;
				// save it as a new entry.
				if (len > 0)
					objList<CommandArg>::add(new CommandArg(p0 + i0, len));
				i0 = i + 1;
			}
			// a colon ':' starts an optional setting associated with a option flag.
			else if (p0[i] == ':')
			{
				len = i - i0;
				if (len == 0)
					return false; // no flag name...
				// save the flag name as a new argument.
				CommandArg *x = new CommandArg(p0 + i0, len);
				i0 = i + 1;
				// a term that follows is the optional setting.
				if (p0[i0] == '"')
				{
					i0++;
					if ((x->_setting = _getTerm<ustring>(L'"', p0, len0, i0)))
						i = i0++;
				}
				else
				{
					if ((x->_setting = _getTerm<ustring>(L' ', p0, len0, i0)))
						i = i0++;
				}
				objList<CommandArg>::add(x);
			}
		}
		// make sure the last argument is picked up.
		len = i - i0;
		if (len > 0)
			objList<CommandArg>::add(new CommandArg(p0 + i0, len));
		return _n;
	}

};
