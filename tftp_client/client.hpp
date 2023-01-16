#include <boost/asio.hpp>

#include <tftp_common/packets.hpp>
#include <tftp_common/serialization.hpp>
#include <tftp_common/parsers.hpp>

#include <string>
#include <fstream>
#include <iostream>

using boost::asio::ip::udp;

namespace tftp_client {

template <typename Iterator, typename Packet, typename ParseFunction>
bool try_parse(Iterator begin, Iterator end, Packet& packet, ParseFunction parseFunction) {
	bool result = parseFunction(begin, end, packet);
	if (result) return true;
	else {
		tftp_common::packets::error err_packet;
		bool result = tftp_common::parsers::parse_error_packet(begin, end, err_packet);
		if (result) throw err_packet;
	}
	return false;
}

class TFTPClient {
	udp::endpoint& receiver_endpoint;
	udp::socket& socket;
public:
	TFTPClient(udp::endpoint& receiver_endpoint, udp::socket& socket): receiver_endpoint(receiver_endpoint), socket(socket) { }
	void send(std::string fromPath, std::string toPath, std::string transferMode) {
		std::ifstream file{fromPath};
		if (!file && file.eof()) return;

		std::vector<unsigned char> send_buffer(1024), recv_buffer(1024);
		std::size_t packet_size = tftp_common::serialize(tftp_common::packets::wrq {
			.filename = toPath,
			.mode = transferMode
		}, send_buffer.begin());
		socket.send_to(boost::asio::buffer(send_buffer, packet_size), receiver_endpoint);

		udp::endpoint sender_endpoint;
		std::size_t bytesRead = socket.receive_from(boost::asio::buffer(recv_buffer), sender_endpoint);

		tftp_common::packets::ack acknowledgment_packet;
		while (file && !file.eof()) {
			try_parse(recv_buffer.begin(), recv_buffer.begin() + bytesRead, acknowledgment_packet, [&]<typename Iterator, typename Packet>(Iterator begin, Iterator end, Packet& packet) {
				return tftp_common::parsers::parse_ack_packet(begin, end, acknowledgment_packet);
			});

			std::vector<unsigned char> data_buffer(512);
			file.read(reinterpret_cast<char*>(data_buffer.data()), 512);
			data_buffer.resize(file.gcount());

			std::size_t packet_size = tftp_common::serialize({
				.block = acknowledgment_packet.block + 1,
				.data = std::move(data_buffer)
			}, send_buffer.begin());
			socket.send_to(boost::asio::buffer(send_buffer, packet_size), receiver_endpoint);
			socket.receive_from(boost::asio::buffer(recv_buffer), sender_endpoint);
		}
	}
};

}