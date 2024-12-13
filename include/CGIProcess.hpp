#ifndef CGIProcess_H
#define CGIProcess_H
#include <string>
#include <vector>
#include "DataType.hpp"
#include "Event.hpp"
#include "HttpResponse.hpp"

class CGIProcess
{
  private:
	  HttpResponse *response;
	  std::vector<std::string> env;
	  std::string cgi_bin;
	  std::string cgi_file;

	  int redirectPipe();
	  void closePipe(int fd[2]);
	  void child_process();
	  void loadEnv();
	  bool IsFileExist();

	  int pipeOut[2];
  public:
	  Proc RunCGIScript(HttpResponse &response);
};

#endif
