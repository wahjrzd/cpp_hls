#pragma once
#include <string>

class Authenticator
{
public:
	Authenticator();
	~Authenticator();

	void setUsernameAndPassword(char const* username, char const* password, bool passwordIsMD5 = false);
	void setRealmAndNonce(char const* realm, char const* nonce);
	std::string createAuthenticatorString(char const* cmd, char const* url);
private:
	std::string computeDigestResponse(char const* cmd, char const* url);
private:
	std::string fRealm;
	std::string fNonce;
	std::string fUserName;
	std::string fPassword;
};

