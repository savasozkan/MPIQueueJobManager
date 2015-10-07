#pragma once

#include "DistributedProcess.h"

/*!
 * This class is main code interface to execute jobs distributedly on a CPU cluster with two additional classes.
 * It uses two process types as Master and Slave and jobs are submitted to Slave processors by Master process
 * one-by-one. OpenMPI is main auxiliary code package/complier.
 *
 * Basically, Master process is to assign jobs to available Slave processors sequentially from a job queue list and
 * communicates with them periodically to update their status/availability.
 * Slave process is responsible to execute a given job and make aware Master process when job is done.
 *
 * For possible usage, two new classes need to be derived from Master and Slave classes and virtual functions
 * must be defined according to problem definition. An example code is provided.
 *
 * For any further question, additional information or contribution, please contact with author via "http://savasozkan.com/"
 */

#define __MASTER 0

/***************************************/
/* Distributed Queue Processing Class. */
/***************************************/

class DistributedQueueProcess
{
public:

	DistributedQueueProcess() {}
	virtual ~DistributedQueueProcess()
	{
		delete m_process;
		MPI_Finalize();
	}

	template <class Master, class Slave>
	void initialize(int argc, char *argv[])
	{
		int rank, processor;

		MPI_Init(&argc,&argv);
		MPI_Comm_size(MPI_COMM_WORLD, &processor);
		MPI_Comm_rank(MPI_COMM_WORLD, &rank);

		if(rank == __MASTER) m_process = new Master;
		else                 m_process = new Slave;

		m_process->initialize(rank, processor);
	}

	void set_conf_param(void* conf) { m_process->set_conf_param(conf); }
	void start() { m_process->run(); }

protected:

	DistributedProcess* m_process;
};
