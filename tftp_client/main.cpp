#include <boost/program_options.hpp>

#include <iostream>

namespace po = boost::program_options;

namespace tftp_client {

enum class method {
	GET, PUT
};

std::istream& operator>>(std::istream& stream, method& object) {
	std::string value; stream >> value;

	if (value == "GET") object = method::GET;
	else if (value == "PUT") object = method::PUT;

	return stream;
}

}

int main(int argc, char* argv[]) {
	if (argc <= 1) {
		std::cout << "Usage: tftp_client host [GET | PUT] source [destination]" << '\n';
	}

	std::string host, source, destination;
	tftp_client::method method;

	po::options_description desc("Allowed options");
	desc.add_options()
		("help", "print help information")
		("host,h", po::value<std::string>(&host), "specifies the local or remote host")
		("source,s", po::value<std::string>(&source), "specifies the file to transfer")
		("method,m", po::value<tftp_client::method>(&method), "method [GET | PUT]")
		("destination,d", po::value<std::string>(&destination), "specifies where to transfer the file")
	;

	po::positional_options_description pos_opts_desc;
	pos_opts_desc
		.add("host", 1).add("method", 1)
		.add("source", 1).add("destination", 1)
	;

	po::variables_map vm;
	po::store(po::command_line_parser(argc, argv).options(desc).positional(pos_opts_desc).run(), vm);
	po::notify(vm);

	if (vm.count("help")) {
		std::cout << desc << '\n';
		return 0;
	}
}
