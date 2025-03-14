#include <boost/asio.hpp>
#include <boost/program_options.hpp>

#include "client.hpp"

#include <cstdlib>
#include <iostream>
#include <string_view>

using namespace tftp::packets;
using boost::asio::ip::udp;
namespace po = boost::program_options;

namespace tftp_client {

enum class method { GET, PUT };

std::istream &operator>>(std::istream &stream, method &object) {
    std::string value;
    stream >> value;

    if (value == "GET")
        object = method::GET;
    else if (value == "PUT")
        object = method::PUT;

    return stream;
}

} // namespace tftp_client

int main(int argc, char *argv[]) {
    if (argc <= 4) {
        std::cout << "Usage: tftp_client host [GET | PUT] source destination [--transfer_mode [netascii | octet]] "
                     "[--option_name name --option_value value]..."
                  << '\n';
        return EXIT_FAILURE;
    }

    std::string host, source, destination, transfer_mode;
    tftp_client::method method;
    std::vector<std::string> option_names, option_values;

    // clang-format off
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "print help information")
        ("host,h", po::value<std::string>(&host), "specifies the local or remote host")
        ("source,s", po::value<std::string>(&source), "specifies the file to transfer")
        ("method,m", po::value<tftp_client::method>(&method), "method [GET | PUT]")
        ("destination,d", po::value<std::string>(&destination), "specifies where to transfer the file")
        ("transfer_mode,t", po::value<std::string>(&transfer_mode), "transfer mode [netascii | octet]")
        ("option_name,n", po::value<std::vector<std::string>>(&option_names), "option name")
        ("option_value,v", po::value<std::vector<std::string>>(&option_values), "option value");
    // clang-format on

    po::positional_options_description pos_opts_desc;
    pos_opts_desc.add("host", 1).add("method", 1).add("source", 1).add("destination", 1);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(desc).positional(pos_opts_desc).run(), vm);
    po::notify(vm);

    if (vm.count("help") || option_names.size() != option_values.size()) {
        std::cout << desc << '\n';
        return EXIT_SUCCESS;
    }

    try {
        boost::asio::io_context io_context;
        udp::resolver resolver(io_context);
        udp::endpoint receiver_endpoint = *resolver.resolve(udp::v4(), host, "69").begin();

        udp::socket socket(io_context);
        socket.open(udp::v4());
        tftp_client::TFTPClient client(resolver, receiver_endpoint, socket);

        if (method == tftp_client::method::PUT) {
            client.send(source, destination, transfer_mode, option_names, option_values);
        } else if (method == tftp_client::method::GET) {
            client.read(source, destination, transfer_mode, option_names, option_values);
        }

        return EXIT_SUCCESS;
    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
    } catch (Error &err) {
        std::cout << "Unexpected error: code = " << err.getErrorCode() << ", message = " << err.getErrorMessage()
                  << '\n';
    }

    return EXIT_FAILURE;
}
