/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hicham <hibenouk@1337.ma>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/09/29 11:55:48 by hicham            #+#    #+#             */
/*   Updated: 2024/09/29 19:34:52 by hicham           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <vector>
#include "Debug.hpp"
#include "VirtualServer.hpp"
#include "Tokenizer.hpp"
#ifdef __cplusplus
extern "C"
#endif
	const char *
	__asan_default_options()
{
	return "detect_leaks=1";
}
void atexist()
{
		system("leaks  webserv");
}
ServerContext* LoadConfig(const char *path)
{
	ServerContext *http = NULL;
	try // ugly but fix the problem
	{
		Tokenizer tokenizer;
		tokenizer.readConfig(path);
		tokenizer.CreateTokens();
		http = new ServerContext();
		tokenizer.parseConfig(http);
	}
	catch (const Debug &e)
	{
		std::cout << e.what() << std::endl;
	}
	catch (const std::bad_alloc &e)
	{
		std::cout << e.what() << std::endl;
	}
	return (http);
}
int main()
{
	// atexit(atexist);
	ServerContext *http = NULL;
	http = LoadConfig("config/nginx.conf");
	delete http;
}
