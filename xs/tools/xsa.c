#include "xsl.h"

#ifdef XSTOOLS
extern int mainXSA(int argc, char* argv[]) ;
int mainXSA(int argc, char* argv[]) 
#else
int main(int argc, char* argv[]) 
#endif
{
	txLinker _linker;
	txLinker* linker = &_linker;
	txLinkerResource** resourceAddress;
	txLinkerScript** scriptAddress;
	int argi;
	txString base = NULL;
  	txString input = NULL;
  	txString output = NULL;
  	txString separator = NULL;
#if mxWindows
	txString url = "\\";
#else
	txString url = "/";
#endif
	char name[C_PATH_MAX];
	char path[C_PATH_MAX];
	txSize size;
	txLinkerResource* resource;
	txLinkerScript* script;
	FILE* file = NULL;

	fxInitializeLinker(linker);
	if (c_setjmp(linker->jmp_buf) == 0) {
		if ((argc == 2) && (c_strcmp(argv[1], "args.txt") == 0)) {
			txString buffer;
			txString string;
			char c;
			file = fopen(argv[1], "r");
			mxThrowElse(file);
			fseek(file, 0, SEEK_END);
			size = (txSize)ftell(file);
			fseek(file, 0, SEEK_SET);
			buffer = fxNewLinkerChunk(linker, size + 1);
			size = (txSize)fread(buffer, 1, size, file);
			buffer[size] = 0;
			fclose(file);
			string = buffer;
			argc = 1;
			while ((c = *string++)) {
				if (isspace(c))
					argc++;
			}
			argv = fxNewLinkerChunk(linker, argc * sizeof(char*));
			for (string = buffer, argc = 1; ; string = NULL, argc++) {
				txString token = strtok(string, " \f\n\r\t\v");
				if (token == NULL)
					break;
				argv[argc] = token;
			}
		}
		c_strcpy(name, "mc");
		resourceAddress = &(linker->firstResource);
		scriptAddress = &(linker->firstScript);
		linker->symbolModulo = 1993;

		for (argi = 1; argi < argc; argi++) {
			if (!c_strcmp(argv[argi], "-b")) {
				argi++;
				if (argi >= argc)
					fxReportLinkerError(linker, "-b: no directory");
				base = fxRealDirectoryPath(linker, argv[argi]);
				if (!base)
					fxReportLinkerError(linker, "-b '%s': directory not found", argv[argi]);
			}
			else if (!c_strcmp(argv[argi], "-n")) {
				argi++;
				if (argi >= argc)
					fxReportLinkerError(linker, "-n: no namespace");
				linker->nameSize = mxStringLength(argv[argi]) + 1;
				linker->name = fxNewLinkerString(linker, argv[argi], linker->nameSize - 1);
			}
			else if (!c_strcmp(argv[argi], "-o")) {
				argi++;
				if (argi >= argc)
					fxReportLinkerError(linker, "-o: no directory");
				output = fxRealDirectoryPath(linker, argv[argi]);
				if (!output)
					fxReportLinkerError(linker, "-o '%s': directory not found", argv[argi]);
			}
			else if (!c_strcmp(argv[argi], "-r")) {
				argi++;
				if (argi >= argc)
					fxReportLinkerError(linker, "-r: no name");
				c_strncpy(name, argv[argi], sizeof(name));
			}
			else if (!c_strcmp(argv[argi], "-u")) {
				argi++;
				if (argi >= argc)
					fxReportLinkerError(linker, "-u: no url");
				url = argv[argi];
				if ((url[0] != '/') && (url[0] != '\\'))
					fxReportLinkerError(linker, "-u: invalid url");
			}
			else {
				input = fxRealFilePath(linker, argv[argi]);
				if (!input)
					fxReportLinkerError(linker, "'%s': file not found", argv[argi]);
				separator = c_strrchr(input, mxSeparator);
				if (!separator)
					fxReportLinkerError(linker, "'%s': invalid extension", input);
				separator = c_strrchr(separator, '.');
				if (!separator)
					fxReportLinkerError(linker, "'%s': invalid extension", input);
				if (!c_strcmp(separator, ".xsb")) {
					*scriptAddress = fxNewLinkerScript(linker, input, &file);
					scriptAddress = &((*scriptAddress)->nextScript);
				}
				else {
					*resourceAddress = fxNewLinkerResource(linker, input, &file);
					resourceAddress = &((*resourceAddress)->nextResource);
				}
			}
		}
		if (!output)
			output = fxRealDirectoryPath(linker, ".");
		if (!base)
			base = output;
		if (!linker->name) {
			linker->nameSize = mxStringLength(name) + 1;
			linker->name = fxNewLinkerString(linker, name, linker->nameSize - 1);
		}
		size = mxStringLength(base);
		script = linker->firstScript;
		while (script) {
			fxBaseScript(linker, script, base, size);
			fxSlashPath(script->path, mxSeparator, url[0]);
			script = script->nextScript;
		}

		linker->symbolTable = fxNewLinkerChunkClear(linker, linker->symbolModulo * sizeof(txLinkerSymbol*));
		fxNewLinkerSymbol(linker, gxIDStrings[0], 0);

		resource = linker->firstResource;
		while (resource) {
			fxBaseResource(linker, resource, base, size);
			resource = resource->nextResource;
		}
		script = linker->firstScript;
		while (script) {
			fxMapScript(linker, script);
			script = script->nextScript;
		}
		fxBufferSymbols(linker);
		fxBufferMaps(linker);
		
		c_strcpy(path, output);
		c_strcat(path, name);
		c_strcat(path, ".xsa");
		fxWriteArchive(linker, path, &file);
	}
	else {
		if (linker->error != C_EINVAL) {
		#if mxWindows
			char buffer[2048];
			FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_MAX_WIDTH_MASK, NULL, linker->error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buffer, sizeof(buffer), NULL);
			fprintf(stderr, "### %s\n", buffer);
		#else
			fprintf(stderr, "### %s\n", strerror(linker->error));
		#endif
		}
	}
	if (file)
		fclose(file);
	fxTerminateLinker(linker);
	return linker->error;
}
