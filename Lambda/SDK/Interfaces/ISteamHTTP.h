#pragma once


#include <cstdint>

enum EHTTPMethod
{
	k_EHTTPMethodInvalid = 0,
	k_EHTTPMethodGET,
	k_EHTTPMethodHEAD,
	k_EHTTPMethodPOST,
	k_EHTTPMethodPUT,
	k_EHTTPMethodDELETE,
	k_EHTTPMethodOPTIONS,
	k_EHTTPMethodPATCH,

	
	

	
	
};




enum EHTTPStatusCode
{
	
	k_EHTTPStatusCodeInvalid = 0,

	
	k_EHTTPStatusCode100Continue = 100,
	k_EHTTPStatusCode101SwitchingProtocols = 101,

	
	k_EHTTPStatusCode200OK = 200,
	k_EHTTPStatusCode201Created = 201,
	k_EHTTPStatusCode202Accepted = 202,
	k_EHTTPStatusCode203NonAuthoritative = 203,
	k_EHTTPStatusCode204NoContent = 204,
	k_EHTTPStatusCode205ResetContent = 205,
	k_EHTTPStatusCode206PartialContent = 206,

	
	k_EHTTPStatusCode300MultipleChoices = 300,
	k_EHTTPStatusCode301MovedPermanently = 301,
	k_EHTTPStatusCode302Found = 302,
	k_EHTTPStatusCode303SeeOther = 303,
	k_EHTTPStatusCode304NotModified = 304,
	k_EHTTPStatusCode305UseProxy = 305,
	
	k_EHTTPStatusCode307TemporaryRedirect = 307,

	
	k_EHTTPStatusCode400BadRequest = 400,
	k_EHTTPStatusCode401Unauthorized = 401, 
	k_EHTTPStatusCode402PaymentRequired = 402, 
	k_EHTTPStatusCode403Forbidden = 403,
	k_EHTTPStatusCode404NotFound = 404,
	k_EHTTPStatusCode405MethodNotAllowed = 405,
	k_EHTTPStatusCode406NotAcceptable = 406,
	k_EHTTPStatusCode407ProxyAuthRequired = 407,
	k_EHTTPStatusCode408RequestTimeout = 408,
	k_EHTTPStatusCode409Conflict = 409,
	k_EHTTPStatusCode410Gone = 410,
	k_EHTTPStatusCode411LengthRequired = 411,
	k_EHTTPStatusCode412PreconditionFailed = 412,
	k_EHTTPStatusCode413RequestEntityTooLarge = 413,
	k_EHTTPStatusCode414RequestURITooLong = 414,
	k_EHTTPStatusCode415UnsupportedMediaType = 415,
	k_EHTTPStatusCode416RequestedRangeNotSatisfiable = 416,
	k_EHTTPStatusCode417ExpectationFailed = 417,
	k_EHTTPStatusCode4xxUnknown = 418, 
	k_EHTTPStatusCode429TooManyRequests = 429,

	
	k_EHTTPStatusCode500InternalServerError = 500,
	k_EHTTPStatusCode501NotImplemented = 501,
	k_EHTTPStatusCode502BadGateway = 502,
	k_EHTTPStatusCode503ServiceUnavailable = 503,
	k_EHTTPStatusCode504GatewayTimeout = 504,
	k_EHTTPStatusCode505HTTPVersionNotSupported = 505,
	k_EHTTPStatusCode5xxUnknown = 599,
};

typedef uint32_t HTTPRequestHandle;
#define INVALID_HTTPREQUEST_HANDLE		0

typedef uint32_t HTTPCookieContainerHandle;
#define INVALID_HTTPCOOKIE_HANDLE		0

typedef uint64_t SteamAPICall_t;
const SteamAPICall_t k_uAPICallInvalid = 0x0;




class ISteamHTTP
{
public:

	
	
	
	
	virtual HTTPRequestHandle CreateHTTPRequest(EHTTPMethod eHTTPRequestMethod, const char* pchAbsoluteURL) = 0;

	
	
	virtual bool SetHTTPRequestContextValue(HTTPRequestHandle hRequest, uint64_t ulContextValue) = 0;

	
	
	
	virtual bool SetHTTPRequestNetworkActivityTimeout(HTTPRequestHandle hRequest, uint32_t unTimeoutSeconds) = 0;

	
	
	virtual bool SetHTTPRequestHeaderValue(HTTPRequestHandle hRequest, const char* pchHeaderName, const char* pchHeaderValue) = 0;

	
	
	
	virtual bool SetHTTPRequestGetOrPostParameter(HTTPRequestHandle hRequest, const char* pchParamName, const char* pchParamValue) = 0;

	
	
	
	
	
	virtual bool SendHTTPRequest(HTTPRequestHandle hRequest, SteamAPICall_t* pCallHandle) = 0;

	
	
	
	virtual bool SendHTTPRequestAndStreamResponse(HTTPRequestHandle hRequest, SteamAPICall_t* pCallHandle) = 0;

	
	
	virtual bool DeferHTTPRequest(HTTPRequestHandle hRequest) = 0;

	
	
	virtual bool PrioritizeHTTPRequest(HTTPRequestHandle hRequest) = 0;

	
	
	
	virtual bool GetHTTPResponseHeaderSize(HTTPRequestHandle hRequest, const char* pchHeaderName, uint32_t* unResponseHeaderSize) = 0;

	
	
	
	virtual bool GetHTTPResponseHeaderValue(HTTPRequestHandle hRequest, const char* pchHeaderName, uint8_t* pHeaderValueBuffer, uint32_t unBufferSize) = 0;

	
	
	virtual bool GetHTTPResponseBodySize(HTTPRequestHandle hRequest, uint32_t* unBodySize) = 0;

	
	
	
	virtual bool GetHTTPResponseBodyData(HTTPRequestHandle hRequest, uint8_t* pBodyDataBuffer, uint32_t unBufferSize) = 0;

	
	
	
	virtual bool GetHTTPStreamingResponseBodyData(HTTPRequestHandle hRequest, uint32_t cOffset, uint8_t* pBodyDataBuffer, uint32_t unBufferSize) = 0;

	
	
	virtual bool ReleaseHTTPRequest(HTTPRequestHandle hRequest) = 0;

	
	
	
	virtual bool GetHTTPDownloadProgressPct(HTTPRequestHandle hRequest, float* pflPercentOut) = 0;

	
	
	
	virtual bool SetHTTPRequestRawPostBody(HTTPRequestHandle hRequest, const char* pchContentType, uint8_t* pubBody, uint32_t unBodyLen) = 0;

	
	
	
	
	
	virtual HTTPCookieContainerHandle CreateCookieContainer(bool bAllowResponsesToModify) = 0;

	
	virtual bool ReleaseCookieContainer(HTTPCookieContainerHandle hCookieContainer) = 0;

	
	virtual bool SetCookie(HTTPCookieContainerHandle hCookieContainer, const char* pchHost, const char* pchUrl, const char* pchCookie) = 0;

	
	virtual bool SetHTTPRequestCookieContainer(HTTPRequestHandle hRequest, HTTPCookieContainerHandle hCookieContainer) = 0;

	
	virtual bool SetHTTPRequestUserAgentInfo(HTTPRequestHandle hRequest, const char* pchUserAgentInfo) = 0;

	
	virtual bool SetHTTPRequestRequiresVerifiedCertificate(HTTPRequestHandle hRequest, bool bRequireVerifiedCertificate) = 0;

	
	
	virtual bool SetHTTPRequestAbsoluteTimeoutMS(HTTPRequestHandle hRequest, uint32_t unMilliseconds) = 0;

	
	virtual bool GetHTTPRequestWasTimedOut(HTTPRequestHandle hRequest, bool* pbWasTimedOut) = 0;
};