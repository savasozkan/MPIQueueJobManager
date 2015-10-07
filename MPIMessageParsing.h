#pragma once

#include "MPIMessageParsing.h"

#define _SLEEP 0.00001

/***********************************/
/* Base-Class for Processing Unit. */
/***********************************/

class DistributedProcess
{
public:

	enum status_type
	{
		eactive = 0,
		einactive = 1,
		ewaiting = 2,
		ebusy = 3,
		eerror = 4
	};

	enum signal_type
	{
		estatus = 0,
		ejob = 1,
		eexit = 2,
		eparam = 3
	};

	DistributedProcess()
	{}

	virtual ~DistributedProcess()
	{}

	virtual void run() =0;
	virtual void set_conf_param(void* param) {}

	void initialize(int rank, int processor) {m_rank=rank; m_processor=processor;}
	int rank() const                         {return m_rank;}
	int processor() const                    {return m_processor;}

protected:

	int m_rank;
	int m_processor;
	MPIMessageParsing m_mpimessage;
};

/*************************/
/* Master Process Class. */
/*************************/

template <typename Job>
class MasterProcess : public DistributedProcess
{
public:

	MasterProcess() {}
	virtual ~MasterProcess() {}

	virtual void run()
	{
		m_v_joblist.clear();
		m_v_signals.clear();
		m_v_signals.resize(m_processor);
		m_v_signals[0] = einactive;

		internal_fill_job_list();

		for(int dest=1; dest<m_processor; dest++)
		{
			status_type message;

			m_mpimessage.send<int>(0, dest, estatus);
			while( not m_mpimessage.probe(dest) );
			m_mpimessage.recv(dest, 0, message);
			m_v_signals[dest] = message;
		}

		internal_set_slave_param();
		start();
	}

	virtual void set_conf_param(void* param)
	{
		m_conf_param = param;
	}

protected:

	virtual void internal_fill_job_list() {}
	virtual void internal_set_slave_param() {}
	virtual void internal_receive_result(int pid) {}

	void start()
	{
		int queue = 0;
		m_n_job = int(m_v_joblist.size());

		bool bcomplete = false;
		while(1)
		{
			for(int p=1; p<m_processor; p++)
			{
				if(m_v_signals[p] == einactive) continue;
				if(queue >= m_n_job){ bcomplete = true; break; }

				control_slave_process(p);

				assign_job(p, queue);
			}

			if(bcomplete) break;
			sleep(_SLEEP);
		}

		exit_stat();
	}

	void control_slave_process(int pid)
	{
		if(m_mpimessage.probe(pid))
		{
			m_mpimessage.recv(pid, 0, m_v_signals[pid]);
			internal_receive_result(pid);
		}
	}

	void exit_stat()
	{
		bool bexit = false;
		while(not bexit)
		{
			bexit = true;
			for(int p=1; p<m_processor; p++)
			{
				switch(m_v_signals[p])
				{
				case ewaiting:
				{
					m_mpimessage.send(0, p, eexit);
					break;
				}
				case ebusy:
				{
					control_slave_process(p);
					bexit = false;
					break;
				}
				default:
					break;
				}
			}

			sleep(_SLEEP);
		}
	}

	void assign_job(const int& rank, int& queue)
	{
		switch(m_v_signals[rank])
		{
		case eactive:
		case ewaiting:
		{
			m_mpimessage.send(0, rank, ejob);
			m_mpimessage.send(m_rank, rank, m_v_joblist[queue]);

			m_v_signals[rank] = ebusy;
			queue += 1;
			break;
		}
		case eerror:
		{
			//todo log file.

			m_v_signals[rank] = ewaiting;
			break;
		}
		default: break;
		}
	}

	int m_n_job;
	vector<Job> m_v_joblist;
	vector<status_type> m_v_signals;
	void* m_conf_param;
};

/***********************/
/* Slave Process Class */
/***********************/

template <typename Job>
class SlaveProcess : public DistributedProcess
{
public:

	SlaveProcess() {}
	virtual ~SlaveProcess() {}

	virtual void run()
	{
		while(true)
		{
			while(not m_mpimessage.probe(0));

			bool bexit = false;
			signal_type signal;
			m_mpimessage.recv(0, m_rank, signal);

			switch(signal)
			{
			case estatus:
			{
				m_mpimessage.send(m_rank, 0, eactive);
				break;
			}
			case ejob:
			{
				status_type status = ewaiting;

				Job job;
				m_mpimessage.recv(0, m_rank, job);

				try         { internal_process(job); }
				catch (...) { status = eerror; }

				m_mpimessage.send(m_rank, 0, status);
				internal_send_result(job);
				break;
			}
			case eexit:
			{
				bexit = true;
				break;
			}
			case eparam:
			{
				internal_set_param();
				break;
			}
			}

			if(bexit) break;
			sleep(_SLEEP);
		}
	}

protected:

	virtual void internal_set_param() {}
	virtual void internal_process(Job& job) {}
	virtual void internal_send_result(const Job& job) {}
};
