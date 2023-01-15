#include <boost/asio.hpp>

#include <tftp_common/packets.hpp>
#include <tftp_common/serialization.hpp>
#include <tftp_common/parsers.hpp>

#include <string>
#include <fstream>

using boost::asio::ip::udp;

namespace tftp_client {

class TFTPClient {
	udp::endpoint& receiver_endpoint;
	udp::socket& socket;
public:
	TFTPClient(udp::endpoint& receiver_endpoint, udp::socket& socket): receiver_endpoint(receiver_endpoint), socket(socket) { }
	void send(std::string fromPath, std::string toPath, std::string transferMode) {
		std::vector<unsigned char> send_buffer, recv_buffer;
		send_buffer.resize(1024); recv_buffer.resize(1024);

		tftp_common::packets::wrq packet = {
			.filename = toPath,
			.mode = transferMode
		};
		std::size_t packet_size = tftp_common::serialize(packet, send_buffer.begin());
		socket.send_to(boost::asio::buffer(send_buffer, packet_size), receiver_endpoint);

		udp::endpoint sender_endpoint;
		socket.receive_from(boost::asio::buffer(recv_buffer), sender_endpoint);

		tftp_common::packets::ack acknowledgment_packet;
		tftp_common::parsers::parse_ack_packet(recv_buffer.begin(), recv_buffer.end(), acknowledgment_packet);

		std::ifstream file{fromPath};
		while (file && !file.eof()) {
			std::vector<unsigned char> data_buffer(512);
			file.read(reinterpret_cast<char*>(data_buffer.data()), 512);
			data_buffer.resize(file.gcount());

			tftp_common::packets::data data_packet = {
				.block = acknowledgment_packet.block + 1,
				.data = std::move(data_buffer)
			};
			std::size_t packet_size = tftp_common::serialize(data_packet, send_buffer.begin());
			socket.send_to(boost::asio::buffer(send_buffer, packet_size), receiver_endpoint);

			socket.receive_from(boost::asio::buffer(recv_buffer), sender_endpoint);
			tftp_common::parsers::parse_ack_packet(recv_buffer.begin(), recv_buffer.end(), acknowledgment_packet);
		}
	}
};

}