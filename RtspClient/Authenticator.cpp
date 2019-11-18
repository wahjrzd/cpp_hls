#include "Authenticator.h"
#include "../WinUtility.h"

Authenticator::Authenticator()
{
}

Authenticator::~Authenticator()
{
}

void Authenticator::setUsernameAndPassword(char const* username, char const* password, bool passwordIsMD5)
{
	fUserName = username;
	fPassword = password;
}

void Authenticator::setRealmAndNonce(char const* realm, char const* nonce)
{
	fRealm = realm;
	if (nonce)
		fNonce = nonce;
}

std::string Authenticator::createAuthenticatorString(char const* cmd, char const* url)
{
	std::string authStr;
	if (!fRealm.empty() && !fUserName.empty() && !fPassword.empty())
	{
		if (!fNonce.empty())
		{
			char buf[1024];
			auto response = computeDigestResponse(cmd, url);
			sprintf_s(buf, "Authorization: Digest username=\"%s\", realm=\"%s\", "
				"nonce=\"%s\", uri=\"%s\", response=\"%s\"\r\n",
				fUserName.c_str(), fRealm.c_str(), fNonce.c_str(), url, response.c_str());
			authStr = buf;
		}
		else
		{
			std::string usernamePassword;
			usernamePassword.append(fUserName);
			usernamePassword.append(":");
			usernamePassword.append(fPassword);
		
			auto response = WinUtility::Base64Encode((BYTE*)usernamePassword.c_str(), usernamePassword.size());
			response.append("\r\n");
			authStr.append("Authorization: Basic ");
			authStr.append(response);
		}
	}
	return authStr;
}

std::string Authenticator::computeDigestResponse(char const* cmd, char const* url)
{
	// The "response" field is computed as:
	//    md5(md5(<username>:<realm>:<password>):<nonce>:md5(<cmd>:<url>))
	// or, if "fPasswordIsMD5" is True:
	//    md5(<password>:<nonce>:md5(<cmd>:<url>))
	std::string ha1;
	std::string ha2;
	std::string ha3;

	ha1.append(fUserName);
	ha1.append(":");
	ha1.append(fRealm);
	ha1.append(":");
	ha1.append(fPassword);

	ha2.append(cmd);
	ha2.append(":");
	ha2.append(url);

	auto data1 = WinUtility::MD5Encode((BYTE*)ha1.c_str(), ha1.size());
	auto data2 = WinUtility::MD5Encode((BYTE*)ha2.c_str(), ha2.size());

	ha3.append(data1);
	ha3.append(":");
	ha3.append(fNonce);
	ha3.append(":");
	ha3.append(data2);

	return  WinUtility::MD5Encode((BYTE*)ha3.c_str(), ha3.size());
}