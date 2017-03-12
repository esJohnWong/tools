#include "Log.h"

int main(int argc, char *argv[])
{
	Log::LogConf conf;
	conf.logPath = "/tmp/testlog";

	Log log(conf);
	log.Info("info log test " + 134 + "," + 456.324);
	log.Warn("warn log test " + 456 + "," + 789.456);
	log.Erro("erro log test " + 789 + "," + 123.345);
	return 0;
}