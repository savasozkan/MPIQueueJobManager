#pragma once

#include "mpi/mpi.h"

#include "unistd.h"
#include "iostream"
#include "vector"
#include "string"

using namespace std;

/***************************/
/* MPIMessageParsing Class */
/***************************/

class MPIMessageParsing
{
public:

	MPIMessageParsing() {}
	virtual ~MPIMessageParsing() {}

	template<typename T>
	void send(int source, int dest, T message)
	{
		Message<T>::send(source, dest, message);
	}

	template<typename T>
	void recv(int source, int dest, T& message)
	{
		Message<T>::recv(source, dest, message);
	}

	int probe(int source)
	{
		int flag;
		int tag = (source == 0) ? 1 : 0;
		MPI_Iprobe(source, tag, MPI_COMM_WORLD, &flag, MPI_STATUS_IGNORE);
		return flag;
	}

protected:

	template<typename T>
	class Message;
};

/*
 * MPI Sending/Receiving Value Message Format.
 */
template<typename T>
class MPIMessageParsing::Message
{
public:

	static void send(int source, int dest, T message)
	{
		int stat;
		int tag = (source == 0) ? 1 : 0;
		size_t size = sizeof(T);

		stat = MPI_Send(&message, size, MPI_CHAR, dest, tag, MPI_COMM_WORLD);
	}

	static void recv(int source, int dest, T& message)
	{
		int stat;
		int tag = (source == 0) ? 1 : 0;
		int size = sizeof(T);
		MPI_Status status;

		stat = MPI_Recv(&message, size, MPI_CHAR, source, tag, MPI_COMM_WORLD, &status);
	}
} ;

/*
 * MPI Sending/Receiving Vector Message Format.
 */
template<typename T>
class MPIMessageParsing::Message<vector<T> >
{
public:

	static void send(int source, int dest, vector<T> message)
	{
		int stat;
		size_t size = sizeof(T);
		int tag = (source == 0) ? 1 : 0;
		int buffer_size = message.size()*size;

		stat = MPI_Send(&buffer_size,          4, MPI_CHAR, dest, tag, MPI_COMM_WORLD);
		stat = MPI_Send(&message[0], buffer_size, MPI_CHAR, dest, tag, MPI_COMM_WORLD);
	}

	static void recv(int source, int dest, vector<T>& message)
	{
		int stat;
		int buffer_size;
		int tag = (source == 0) ? 1 : 0;
		size_t size = sizeof(T);
		MPI_Status status;

		message.clear();

		stat = MPI_Recv(&buffer_size, sizeof(int), MPI_CHAR, source, tag, MPI_COMM_WORLD, &status);
		message.resize(buffer_size/size);
		stat = MPI_Recv(&message[0],  buffer_size, MPI_CHAR, source, tag, MPI_COMM_WORLD, &status);
	}
} ;

/*
 *   MPI Sending/Receiving String Message Format.
 */
template<>
class MPIMessageParsing::Message<string>
{
public:

	static void send(int source, int dest, string message)
	{
		int stat;
		int tag = (source == 0) ? 1 : 0;
		int buffer_size = message.size();

		stat = MPI_Send(&buffer_size,                     4, MPI_CHAR, dest, tag, MPI_COMM_WORLD);
		stat = MPI_Send((void*)message.c_str(), buffer_size, MPI_CHAR, dest, tag, MPI_COMM_WORLD);
	}

	static void recv(int source, int dest, string& message)
	{
		int stat;
		int tag = (source == 0) ? 1 : 0;
		int buffer_size;
		MPI_Status status;

		message.clear();

		stat = MPI_Recv(&buffer_size,                     4, MPI_CHAR, source, tag, MPI_COMM_WORLD, &status);
		message.resize(buffer_size);
		stat = MPI_Recv((void*)message.c_str(), buffer_size, MPI_CHAR, source, tag, MPI_COMM_WORLD, &status);
	}
} ;
