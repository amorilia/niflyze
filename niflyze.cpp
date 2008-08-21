/* Copyright (c) 2005, NIF File Format Library and Tools
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

   * Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.

   * Redistributions in binary form must reproduce the above
     copyright notice, this list of conditions and the following
     disclaimer in the documentation and/or other materials provided
     with the distribution.

   * Neither the name of the NIF File Format Library and Tools
     project nor the names of its contributors may be used to endorse
     or promote products derived from this software without specific
     prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE. */

#include "niflib.h"
#include "obj/NiObject.h"
using namespace Niflib;

#include <iomanip>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <sstream>

//#define USE_NIFLIB_DLL
//#define TEST_WRITE
//#define TEST_INDIVIDUAL_CLONE
//#define TEST_TREE_CLONE

// _WIN32 will detect windows on most compilers
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define endl "\r\n"
#include "windows.h"
#else
#include <dirent.h>
#include <regex.h>
#include <time.h>
#endif

using namespace std;

int main( int argc, char* argv[] );
bool HasBlockType( vector<NiObjectRef> blocks, string const & block_type );
void PrintHelpInfo( ostream & out );
void PrintTree( NiObjectRef block, int indent, ostream & out );
string FixLineEnds( const string & in );
void ParseVersionLimit( char* str, unsigned& version_begin, unsigned& version_end);

int main( int argc, char* argv[] ){
	bool block_match = false;
	bool exclusive_mode = false;
	bool use_start_dir = false;
	bool verbose = false;
	bool test_only = false;
	char * block_match_string = "";
	char * in_file = "*.nif";  //C_Templar_M_G_skirt
	char * out_file = "niflyze.txt";
	char * start_dir = ".";
    unsigned version_begin=0;
    unsigned version_end=-1;

	////Temporary//
	//start_dir = "C:\\Documents and Settings\\Shon\\My Documents\\Modding\\Morrowind\\Official Nifs";
	//use_start_dir = true;
	///////////////

	bool help_flag = false;
	for (int i = 0; i < argc; ++i ) {
		//--Look for switches--//

		// Input File
		if ( strcmp(argv[i], "-i") == 0 ) {
			
			//If not already on the last argument
			if (i != argc - 1) {
				//Move to next argument and record input file name
				i++;
				in_file = argv[i];
			}
		}

		// Output File
		if ( strcmp(argv[i], "-o") == 0  ) {
			
			//If not already on the last argument
			if (i != argc - 1) {
				//Move to next argument and record output file name
				i++;
				
				out_file = argv[i];
			}
		}

		// Start Directory
		if ( strcmp(argv[i], "-p") == 0  ) {
			
			//If not already on the last argument
			if (i != argc - 1) {
				//Move to next argument and record output file name
				i++;
				start_dir = argv[i];
				use_start_dir = true;
			}
		}

		// Block Match
		if ( strcmp(argv[i], "-b") == 0  ) {
			
			//If not already on the last argument
			if (i != argc - 1) {
				//Move to next argument and record output file name
				i++;
				block_match_string = argv[i];
				block_match = true;
			}
		}

        // Limit Version
        if ( strcmp(argv[i], "-l") == 0  ) {

            //If not already on the last argument
            if (i != argc - 1) {
                //Move to next argument and record output file name
                i++;
                ParseVersionLimit(argv[i],version_begin,version_end);
            }
        }

		// Verbose mode
		if ( strcmp(argv[i], "-v") == 0  ) {
			verbose = true;
		}

		// Test Only mode (no file writing, but call asString anyway)
		if ( strcmp(argv[i], "-t") == 0  ) {
			test_only = true;
		}

		// Exclusive mode
		if ( strcmp(argv[i], "-x") == 0  ) {
			exclusive_mode = true;
		}

		// Help
		if ( strcmp(argv[i], "-?") == 0 || strcmp(argv[i], "-h") == 0 ) {
			help_flag = true;
		}
	}	

	// if there are no arguments, show help
	//if ( argc == 1 ) {
	//	help_flag = true;
	//}

	if ( help_flag == true ) {
		//Help request intercepted
		PrintHelpInfo( cout );
		return 1;
	}

#ifdef _WIN32
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;
#else
	DIR *dir;
	dirent *pdirent;
	string pattern(in_file);
	string tmp;
	for ( string::const_iterator it = pattern.begin(); it != pattern.end(); it++) {
		if ( *it == '.' )
			tmp += "\\.";
		else if ( *it == '*' )
			tmp += ".*";
		else tmp += *it;
	};
	pattern = tmp + "$";
	
	regex_t regex;
	if (regcomp(&regex, pattern.c_str(), REG_EXTENDED|REG_NOSUB) == -1)
		cout << "Failed to compile regular expression." << endl;
#endif

	//Open output file
	ofstream out( out_file, ofstream::binary );
	out.setf(ios::fixed, ios::floatfield);
	out << setprecision(1);
	cout.setf(ios::fixed, ios::floatfield);
	cout << setprecision(2);

#ifdef _WIN32
	if ( use_start_dir )
		SetCurrentDirectory(start_dir);
#else
	dir = opendir(start_dir);
	if (dir == NULL) {
		cout << "Directory not found." << endl;
		return -1;
	};
#endif

	//Start Timer
#ifdef _WIN32
	DWORD start_time = GetTickCount();
#else
	clock_t start_time = clock();
#endif
	unsigned int count = 0;

	map<string, vector<string> > versions;

	// Find files
#ifdef _WIN32
	hFind = FindFirstFile(in_file, &FindFileData);
	if (hFind == INVALID_HANDLE_VALUE) {
#else
	pdirent = readdir(dir);
	while (pdirent != NULL) {
		if (strcmp(".", pdirent->d_name) == 0 || strcmp("..", pdirent->d_name) == 0) {
			pdirent = readdir(dir);
			continue;
		};
		if (regexec(&regex, pdirent->d_name, 0, NULL, 0) == 0) break;
		pdirent = readdir(dir);
    };
	if (pdirent == NULL) {
#endif
		cout << "No files found." << endl;
		return 1;
	} else {

		bool file_found = true;
		while (file_found == true) {
			//--Read NIF File--//
#ifdef _WIN32
			string current_file = FindFileData.cFileName;
#else
			string current_file = string(start_dir) + "/" + string(pdirent->d_name);
#endif
			//--Open File--//
			ifstream in( current_file.c_str(), ifstream::binary );

			cout << "Reading "  << current_file;

			vector< NiObjectRef > blocks;
			NiObjectRef root;
            unsigned int ver;
			try {
				////Show block tree
				//root = ReadNifTree( current_file );
				//PrintTree( root, 0, out );
				//out << endl;

				ver = GetNifVersion( current_file );
				cout << " (" << FormatVersionString(ver) << dec << ")...";

                if ( (ver < version_begin) || (ver > version_end) ) cout << "skipped...";
                else if ( IsSupportedVersion(ver) == false ) cout << "unsupported...";
				else if ( ver == VER_INVALID ) cout << "invalid...";
				else {
					
                    blocks = ReadNifList( current_file );
 					//blocks.push_back( ReadNifTree( current_file ) );

	
					////Add files to the lists if they have that specific type of texture
					//for ( unsigned int i = 0; i < blocks.size(); ++i ) {
					//	if ( blocks[i]->GetBlockType() == "NiTexturingProperty" ) {
					//		//if ( blocks[i]->GetAttr("Dark Texture")->asTexDesc().isUsed == true ) {
					//		//	dark_tx.push_back( current_file );
					//		//}
					//		//if ( blocks[i]->GetAttr("Detail Texture")->asTexDesc().isUsed == true ) {
					//		//	detail_tx.push_back( current_file );
					//		//}
					//		//if ( blocks[i]->GetAttr("Decal Texture")->asTexDesc().isUsed == true ) {
					//		//	decal_tx.push_back( current_file );
					//		//}
					//		//if ( blocks[i]->GetAttr("Decal Texture 2")->asTexDesc().isUsed == true ) {
					//		//	decal2_tx.push_back( current_file );
					//		//}
					//		//if ( blocks[i]->GetAttr("Glow Texture")->asTexDesc().isUsed == true ) {
					//		//	glow_tx.push_back( current_file );
					//		//}
					//		//if ( blocks[i]->GetAttr("Gloss Texture")->asTexDesc().isUsed == true ) {
					//		//	gloss_tx.push_back( current_file );
					//		//}
	
					//		//Try to copy the base texture to another slot
					//Texture t = blocks[i]["Base Texture"]->asTexDesc();
					//		float pi = 3.141592653589793f;
					//		float angle = 0.25f * pi;
					//		
					//		t.bmLumaOffset = 45.0f;
					//		t.bmLumaScale = 45.0f;
					//		t.bmMatrix[0][0] = cos(angle);		t.bmMatrix[0][1] = -sin(angle);
					//		t.bmMatrix[1][0] = sin(angle);		t.bmMatrix[1][1] = cos(angle);
					//		blocks[i]["Broken Texture?"]->Set(t);
	
					//		//Create a texture source for the alpha block
					//		blk_ref src_prop = CreateBlock( "NiSourceTexture" );
					//		TextureSource ts;
					//		ts.useExternal = true;
					//		ts.fileName = "snowwallbump.tga";
					//		ts.unknownByte = 0;
					//		src_prop["Texture Source"]->Set(ts);
					//		src_prop["Pixel Layout"]->Set(4);
					//		src_prop["Use Mipmaps"]->Set(0);
					//		src_prop["Alpha Format"]->Set(0);
					//		src_prop["Unknown Byte"]->Set(1);
					//		blk_link l;
					//		l.attr = blocks[i]["Broken Texture?"];
					//		l.block = src_prop;
					//		blocks[i]->AddLink( l );
					//		
					//		//t.isUsed = false;
					//		//blocks[i]->GetAttr("Base Texture")->Set(t);
	
					//		//blk_link l = blocks[i]->GetLink( blocks[i]->GetAttr("Base Texture") );
					//		//l.attr = blocks[i]->GetAttr("Broken Texture?");
					//		//blocks[i]->AddLink( l );
					//	}
					//	//if ( blocks[i]->GetBlockType() == "NiSourceTexture" ) {
					//	//	blocks[i]->GetAttr("Pixel Layout")->Set(4);
					//	//}
					//	if ( blocks[i]->GetBlockType() == "NiMaterialProperty" ) {
					//		blocks[i]["Specular Color"]->Set( 1.0f, 1.0f, 1.0f );
					//		blocks[i]["Glossiness"]->Set( 20.0f );
					//	}
					//	if ( blocks[i]->GetBlockType() == "NiTriShape" ) {
					//		blk_ref spec_prop = CreateBlock( "NiSpecularProperty" );
					//		spec_prop["Flags"]->Set(1);
					//		blk_link l;
					//		l.attr = blocks[i]["Properties"];
					//		l.block = spec_prop;
					//		blocks[i]->AddLink( l );
	
					//		blk_ref render_prop = CreateBlock( "NiRendererSpecificProperty" );
					//		render_prop["Flags"]->Set(int(0xFFFFFFFF));
					//		l.attr = blocks[i]["Properties"];
					//		l.block = render_prop;
					//		blocks[i]->AddLink( l );
					//	}
	
					//}
	
					//Increment file count
					count++;
	
					//--Output Analysis--//
					if ( !test_only ) {
						if ( !block_match || HasBlockType( blocks, string(block_match_string) ) ) {
							cout << "writing...";
							if (exclusive_mode) {
								for ( unsigned int i = 0; i < blocks.size(); ++i ) {
								if ( blocks[i]->GetType().GetTypeName() == string(block_match_string) ) {
									out << "====[ " << current_file << " | " << blocks[i]->GetIDString() << " ]====" << endl
										<< FixLineEnds( blocks[i]->asString( verbose ) )
										<< endl;
								}
		
								//IPixelData * pix_data = (IPixelData*)blocks[i]->QueryInterface( ID_PIXEL_DATA );
								//if ( pix_data != NULL ) {
								//	PixelFormat pf = pix_data->GetPixelFormat();
								//	if ( pf == PX_FMT_RGB8 || pf == PX_FMT_RGBA8 ) {
								//		cout << endl << "Texture found:  " << pix_data->GetWidth() << "x" << pix_data->GetHeight() << endl;
								//		vector<Color4> colors = pix_data->GetColors();
								//		cout << "Sending colors back to NiPixelData block." << endl;
								//		pix_data->SetColors( colors, true );
								//		cout << "Displaying NiPixelData block." << endl;
								//		cout << blocks[i]->asString();
								//		cin.get();
								//	}
								//}
							}
							} else {
								for ( unsigned int i = 0; i < blocks.size(); ++i ) {
									out << "====[ " << current_file << " | " << blocks[i]->GetIDString() << " ]====" << endl
										<< FixLineEnds( blocks[i]->asString( verbose ) )
										<< endl;
								}
							}
						}
					} else {
						//Test only mode.  Simply call asSring function on each block
						//cout << "testing...";
						//for ( unsigned int i = 0; i < blocks.size(); ++i ) {
						//	blocks[i]->asString( verbose );
						//}
					}
				};
				cout << "done" << endl;

#ifdef TEST_WRITE
				////Test Write Function
				cout << endl << "Writing Nif File" << endl;
				//string output_nif_file = "C:\\Documents and Settings\\Shon\\My Documents\\Visual Studio Projects\\Niflyze\\Release\\TEST.NIF";
				string output_nif_file = "/tmp/TEST.NIF";
				WriteNifTree( output_nif_file, blocks[0], NifInfo(ver) );
				blocks = ReadNifList( output_nif_file );
				for ( unsigned int i = 0; i < blocks.size(); ++i ) {
					out << "====[ " << current_file << " | " << blocks[i]->GetIDString() << " ]====" << endl
						<< blocks[i]->asString( verbose )
						<< endl;
				};
				cin.get();
#endif

#ifdef TEST_INDIVIDUAL_CLONE
				for ( unsigned int i = 0; i < blocks.size(); ++i ) {
					//Test Clone Function
					cout << "==[Original | " << blocks[i]->GetIDString() << "]==" << endl << endl
						 << blocks[i]->asString() << endl;
					NiObjectRef clone = blocks[i]->Clone( ver );
					cout << "==[Clone | " << clone->GetIDString() << "]==" << endl << endl
						 << clone->asString() << endl;
				}
				cin.get();
#endif

#ifdef TEST_TREE_CLONE
				//Try to clone the whole tree, assuming blocks[0] is the root
				NiObjectRef clone_root = CloneNifTree( blocks[0], ver );
				//Print out cloned tree
				PrintTree( clone_root, 0,  cout );
				cin.get();
#endif

			}
			catch( exception & e ) {
				cout << endl << "Error: " << e.what() << endl;
				return 0;
			}
			catch( ... ) {
				cout << endl << "Unknown Exception." << endl;
				return 0;
			}

			//Clear out current file
			blocks.clear();

			//--Find Next File--//
#ifdef _WIN32
			file_found = ( FindNextFile(hFind, &FindFileData) != 0 );
#else
			pdirent = readdir(dir);
			while (pdirent != NULL) {
				if (regexec(&regex, pdirent->d_name, 0, NULL, 0) == 0) break;
				pdirent = readdir(dir);
			};
			if (pdirent == NULL) file_found = false; else file_found = true;
#endif
		}
	}

	////Print out file/version correspondance
	//map<string, vector<string> >::iterator it;
	//for (it = versions.begin(); it != versions.end(); ++it ) {
	//	out << endl
	//		<< "===[" << it->first << "]===" << endl;

	//	vector<string>::iterator it2;
	//	for (it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
	//		out << "   " << *it2 << endl;
	//	}

	//}
	
	////Prune list
	//sort< vector<string>::iterator >(versions.begin(), versions.end());
	//unique<  vector<string>::iterator  >(versions.begin(), versions.end());

		////Print out list
	//out << "Versions:" << endl;
	//for ( unsigned int i = 0; i < versions.size(); ++i ) {
	//	out << "   " << versions[i] << endl;
	//}

	////Prune lists
	//sort(dark_tx.begin(), dark_tx.end());
	//unique(dark_tx.begin(), dark_tx.end());

	//sort(detail_tx.begin(), detail_tx.end());
	//unique(detail_tx.begin(), detail_tx.end());

	//sort(decal_tx.begin(), decal_tx.end());
	//unique(decal_tx.begin(), decal_tx.end());

	//sort(decal2_tx.begin(), decal2_tx.end());
	//unique(decal2_tx.begin(), decal2_tx.end());

	//sort(glow_tx.begin(), glow_tx.end());
	//unique(glow_tx.begin(), glow_tx.end());

	//sort(gloss_tx.begin(), gloss_tx.end());
	//unique(gloss_tx.begin(), gloss_tx.end());

	////Print out lists

	//out << "Dark Texture:" << endl;
	//for (unsigned int i = 0; i < dark_tx.size(); ++i) {
	//	out << "   " << dark_tx[i] << endl;
	//}
	//out << endl;
	//out << "Detail Texture:" << endl;
	//for (unsigned int i = 0; i < detail_tx.size(); ++i) {
	//	out << "   " << detail_tx[i] << endl;
	//}
	//out << endl;
	//out << "Decal Texture:" << endl;
	//for (unsigned int i = 0; i < decal_tx.size(); ++i) {
	//	out << "   " << decal_tx[i] << endl;
	//}
	//out << "Decal Texture 2:" << endl;
	//for (unsigned int i = 0; i < decal2_tx.size(); ++i) {
	//	out << "   " << decal2_tx[i] << endl;
	//}
	//out << endl;
	//out << "Glow Texture:" << endl;
	//for (unsigned int i = 0; i < glow_tx.size(); ++i) {
	//	out << "   " << glow_tx[i] << endl;
	//}
	//out << endl;
	//out << "Gloss Texture:" << endl;
	//for (unsigned int i = 0; i < gloss_tx.size(); ++i) {
	//	out << "   " << gloss_tx[i] << endl;
	//}

	out.close();

	cout << "Done!" << endl;
	cout << "Objects in memory:  " << NiObject::NumObjectsInMemory() << endl;
#ifdef _WIN32
	DWORD end_time = GetTickCount();
	float time_taken = float(end_time - start_time) / 1000.0f;
#else
	closedir(dir);
	clock_t end_time = clock();
	float time_taken = float(end_time - start_time) / CLOCKS_PER_SEC;
#endif
	cout << count << " files analyzed." << endl
		 << "Total processing time:  " << time_taken << " seconds" << endl; 
	return 1;
}

bool HasBlockType( vector<NiObjectRef> blocks, string const & block_type ) {
	for ( unsigned int i = 0; i < blocks.size(); ++i ) {
		if ( blocks[i]->GetType().GetTypeName() == block_type )
			return true;
	}
	return false;
}

void PrintTree( NiObjectRef block, int indent, ostream & out ) {
	//Print indent
	for (int i = 0; i < indent; ++i) {
		out << "   ";
	}

	//Print Block
	out << "* " << block << endl;

	//Call this function for all children of this block with a higher indent
	list<NiObjectRef> links = block->GetRefs();
	list<NiObjectRef>::iterator it;
	for ( it = links.begin(); it != links.end(); ++it ) {
		//if ( (*it)->GetParent() == block )
		PrintTree( *it, indent + 1, out );
	}
}

void PrintHelpInfo( ostream & out ) {
	out << "Usage:  niflyze -switch_1 value_1 -switch_2 value_2 ... switch_n value_n" << endl
		<< "Example:  niflyze -i model.nif -o output.txt" << endl << endl
		<< "Switches:" << endl
		<< "-? [Help]" << endl
		<< "   Displays This Information" << endl << endl
		<< "-i [Input File]" << endl
		<< "   The nif file(s) to analyze.  Can include wildcards." << endl
		<< "   Example: model.nif" << endl
		<< "   Default: *.nif" << endl << endl
		<< "-o [Output File]" << endl
		<< "   The text file to output analysis of NIF file(s) to." << endl
		<< "   Example: -o output.txt" << endl
		<< "   Default:  niflyze.txt" << endl << endl
		<< "-p [Search Path]" << endl
		<< "   Specifies the directory to search for files matching the input file name." << endl
		<< "   If no path is specified, the present directory is searched.  Don't specify" << endl
		<< "   a path in the in file or the read will fail." << endl
		<< "   Example:  -p \"C:\\Program Files\\\"" << endl << endl
		<< "-b [Block Match]" << endl
		<< "   Causes niflyze to output information only for files which contain the" << endl
		<< "   specified block type. Default behavior is to output information about" << endl
		<< "   all files read." << endl
		<< "   Example: -b NiNode" << endl << endl
		<< "-v [Verbose]" << endl
		<< "   Causes niflyze to output all complete data arrays such as vertices and" << endl
		<< "   unknown data areas in hex form. Default behavior is not to output this" << endl
		<< "   information." << endl
		<< "   Example: -v" << endl << endl
		<< "-x [Exclusive Mode]" << endl
		<< "   Causes niflyze to output only the blocks that match the block type" << endl
		<< "   specified with the -b switch.  Normally the whole file that contians" << endl
		<< "   the block is output." << endl
		<< "   Example: -x" << endl << endl
        << "-t [Test only]" << endl
        << "   Just check if .nif file(s) are read and parseable." << endl
        << "   Example: -t" << endl << endl
        << "-l [Limit version]" << endl
        << "   Process only files with matching version numbers." << endl
        << "   Takes a hexadezimal span as parameter." << endl
        << "   Example: -l 03010000-04000000" << endl
        << "   Example: -l 0a000000-" << endl;
}

string FixLineEnds( const string & in ) {
#ifdef _WIN32
	//Handle strange Windows line ends
	stringstream ss;
	for ( unsigned int j = 0; j < in.size(); ++j ) {
		if ( in[j] == '\n' ) {
			ss << endl;
		} else {
			ss << in[j];
		}
	}

	return ss.str();
#else
	//Just return the string given
	return in;
#endif
}


#define HEXNUM(x) ((x)>='0'&&(x)<='9')?(x)-'0':(((x)>='a'&&(x)<='f')?(x)+10-'a':(((x)>='A'&&(x)<='F')?(x)+10-'A':-1))

unsigned ParseHex(char*& hex)
{
    unsigned res = 0;

    int r = HEXNUM(*hex);
    while (r>=0)
    {
        res <<= 4;
        res |=r;
        ++hex;
        r = HEXNUM(*hex);
    } 
    return res;
}

void ParseVersionLimit( char* str, unsigned& version_begin, unsigned& version_end)
{
    version_begin =0;
    version_end = -1;

    if (*str=='-')
    {
        version_end = ParseHex(++str);
    }
    else
    {
        version_begin = ParseHex(str);
        if (*str!='-')  version_end = version_begin;
        else
        {
            version_end = ParseHex(++str);
            if (version_end==0) version_end = -1;
        }
    }
}
