#include <fast_cgi/fast_cgi.hpp>

class responder : public fast_cgi::responder
{
public:
	virtual status_code_type run() override
	{
		using namespace fast_cgi::manipulator;

		output() << "Content-type: text/html" << feed << feed;
		output() << "<html>"
				 << "<h1>"
				 << "Hello, World!"
				 << "</h1>"
				 << "Your URI: " << params()["REQUEST_URI"] << "</html>";
		
		return 0;
	}
};

int main()
{
	// create server
	fast_cgi::protocol protocol(std::make_shared<tcp_connector>("0.0.0.0", 6545));

	protocol.set_role<responder>();

	protocol.run();
	protocol.join();
}
