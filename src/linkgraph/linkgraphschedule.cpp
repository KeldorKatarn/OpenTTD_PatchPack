/* $Id$ */

/*
* This file is part of OpenTTD.
* OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
* OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
*/

/** @file linkgraphschedule.cpp Definition of link graph schedule used for cargo distribution. */

#include "../stdafx.h"
#include "linkgraphschedule.h"

#include "../command_func.h"
#include "demands.h"
#include "flowmapper.h"
#include "init.h"
#include "mcf.h"

#include <algorithm>

#include "../safeguards.h"

/**
* Static instance of LinkGraphSchedule.
* Note: This instance is created on task start.
*       Lazy creation on first usage results in a data race between the CDist threads.
*/
/* static */
LinkGraphSchedule LinkGraphSchedule::instance;

/**
* Start the next job(s) in the schedule.
*
* The cost estimate of a link graph job is C ~ N^2 log N, where
* N is the number of nodes in the job link graph.
* The cost estimate is summed for all running and scheduled jobs to form the total cost estimate T = sum C.
* The nominal cycle time (in recalc intervals) required to schedule all jobs is calculated as S = 1 + log_2 T.
* Hence the nominal duration of an individual job (in recalc intervals) is D = ceil(S * C / T)
* The cost budget for an individual call to this method is given by T / S.
*
* The purpose of this algorithm is so that overall responsiveness is not hindered by large numbers of small/cheap
* jobs which would previously need to be cycled through individually, but equally large/slow jobs have an extended
* duration in which to execute, to avoid unnecessary pauses.
*/
void LinkGraphSchedule::SpawnNext()
{
	if (this->schedule.empty()) return;

	GraphList schedule_to_back;
	uint64 total_cost = 0;

	for (auto link_graph = this->schedule.begin(); link_graph != this->schedule.end();) {
		const auto current_link_graph = link_graph;
		++link_graph;
		const LinkGraph* graph = *current_link_graph;

		if (graph->Size() < 2) {
			schedule_to_back.splice(schedule_to_back.end(), this->schedule, current_link_graph);
		} else {
			total_cost += graph->CalculateCostEstimate();
		}
	}

	for (auto& job : this->running) {
		total_cost += job->Graph().CalculateCostEstimate();
	}

	const uint scaling = 1 + FindLastBit(total_cost);
	const uint64 cost_budget = total_cost / scaling;
	uint64 used_budget = 0;
	std::vector<LinkGraphJobGroup::JobInfo> jobs_to_execute;

	while (used_budget < cost_budget && !this->schedule.empty()) {
		LinkGraph* link_graph = this->schedule.front();
		assert(link_graph == LinkGraph::Get(link_graph->index));
		this->schedule.pop_front();
		const uint64 cost = link_graph->CalculateCostEstimate();
		used_budget += cost;

		if (LinkGraphJob::CanAllocateItem()) {
			const auto duration_multiplier = static_cast<uint>(CeilDivT<uint64_t>(scaling * cost, total_cost));
			std::unique_ptr<LinkGraphJob> job(new LinkGraphJob(*link_graph, duration_multiplier));
			jobs_to_execute.emplace_back(job.get(), static_cast<uint>(cost));

			if (this->running.empty() || job->JoinDateTicks() >= this->running.back()->JoinDateTicks()) {
				this->running.push_back(std::move(job));
			} else {
				// find right place to insert
				const auto upper_bound = std::upper_bound(this->running.begin(), this->running.end(), job->JoinDateTicks(), [](Ticks a, const std::unique_ptr<LinkGraphJob>& b)
				{
					return a < b->JoinDateTicks();
				});

				this->running.insert(upper_bound, std::move(job));
			}
		} else {
			NOT_REACHED();
		}
	}

	this->schedule.splice(this->schedule.end(), schedule_to_back);

	LinkGraphJobGroup::ExecuteJobSet(std::move(jobs_to_execute));
}

/**
* Join the next finished job, if available.
*/
bool LinkGraphSchedule::IsJoinWithUnfinishedJobDue() const
{
	for (const auto& job : this->running) {
		if (!job->IsFinished(1)) {
			// job is not due to be joined yet.
			return false;
		}

		if (!job->IsJobCompleted()) {
			// job is due to be joined, but is not completed.
			return true;
		}
	}

	return false;
}

/**
* Join the next finished job, if available.
*/
void LinkGraphSchedule::JoinNext()
{
	while (!this->running.empty()) {
		if (!this->running.front()->IsFinished()) return;

		std::unique_ptr<LinkGraphJob> next = std::move(this->running.front());
		this->running.pop_front();
		const LinkGraphID index = next->LinkGraphIndex();
		next->FinaliseJob(); // joins the thread and finalises the job
		assert(!next->IsJobAborted());
		next.reset();

		if (LinkGraph::IsValidID(index)) {
			LinkGraph* link_graph = LinkGraph::Get(index);
			this->Unqueue(link_graph); // Unqueue to avoid double-queueing recycled IDs.
			this->Queue(link_graph);
		}
	}
}

/**
* Run all handlers for the given Job. This method is tailored to
* ThreadObject::New.
* @param j Pointer to a link graph job.
*/
/* static */
void LinkGraphSchedule::Run(void* j)
{
	const auto job = static_cast<LinkGraphJob *>(j);

	for (auto& handler : instance.handlers) {
		if (job->IsJobAborted()) return;

		handler->Run(*job);
	}

	/*
	* Note that this it not guaranteed to be an atomic write and there are no memory barriers or other protections.
	* Readers of this variable in another thread may see an out of date value.
	* However this is OK as this will only happen just as a job is completing, and the real synchronisation is provided
	* by the thread join operation. In the worst case the main thread will be paused for longer than strictly necessary before
	* joining.
	* This is just a hint variable to avoid performing the join excessively early and blocking the main thread.
	*/

#if defined(__GNUC__) || defined(__clang__)
	__atomic_store_n(&(job->job_completed), true, __ATOMIC_RELAXED);
#else
	job->job_completed = true;
#endif
}

/**
* Start all threads in the running list. This is only useful for save/load.
* Usually threads are started when the job is created.
*/
void LinkGraphSchedule::SpawnAll()
{
	std::vector<LinkGraphJobGroup::JobInfo> jobs_to_execute;

	for (auto& job : this->running) {
		jobs_to_execute.emplace_back(job.get());
	}

	LinkGraphJobGroup::ExecuteJobSet(std::move(jobs_to_execute));
}

/**
* Clear all link graphs and jobs from the schedule.
*/
/* static */
void LinkGraphSchedule::Clear()
{
	for (auto& job : instance.running) {
		job->AbortJob();
	}

	instance.running.clear();
	instance.schedule.clear();
}

/**
* Shift all dates (join dates and edge annotations) of link graphs and link
* graph jobs by the number of days given.
* @param interval Number of days to be added or subtracted.
*/
void LinkGraphSchedule::ShiftDates(int interval)
{
	LinkGraph* lg;
	FOR_ALL_LINK_GRAPHS(lg) lg->ShiftDates(interval);

	LinkGraphJob* lgj;
	FOR_ALL_LINK_GRAPH_JOBS(lgj) lgj->ShiftJoinDate(interval);
}

/**
* Create a link graph schedule and initialize its handlers.
*/
LinkGraphSchedule::LinkGraphSchedule()
{
	this->handlers[0].reset(new InitHandler);
	this->handlers[1].reset(new DemandHandler);
	this->handlers[2].reset(new MCFHandler<MCF1stPass>);
	this->handlers[3].reset(new FlowMapper(false));
	this->handlers[4].reset(new MCFHandler<MCF2ndPass>);
	this->handlers[5].reset(new FlowMapper(true));
}

/**
* Delete a link graph schedule and its handlers.
*/
LinkGraphSchedule::~LinkGraphSchedule()
{
	this->Clear();
}

LinkGraphJobGroup::LinkGraphJobGroup(constructor_token token, std::vector<LinkGraphJob *> jobs) :
	jobs(std::move(jobs)) { }

void LinkGraphJobGroup::SpawnThread()
{
	ThreadObject* thread = nullptr;

	/**
	* Spawn a thread if possible and run the link graph job in the thread. If
	* that's not possible run the job right now in the current thread.
	*/
	if (ThreadObject::New(&Run, this, &thread, "ottd:linkgraph")) {
		this->thread.reset(thread);

		for (auto& job : this->jobs) {
			job->SetJobGroup(this->shared_from_this());
		}
	} else {
		this->thread.reset();
		/* Of course this will hang a bit.
		* On the other hand, if you want to play games which make this hang noticably
		* on a platform without threads then you'll probably get other problems first.
		* OK:
		* If someone comes and tells me that this hangs for him/her, I'll implement a
		* smaller grained "Step" method for all handlers and add some more ticks where
		* "Step" is called. No problem in principle. */
		Run(this);
	}
}

void LinkGraphJobGroup::JoinThread()
{
	if (!this->thread || this->joined_thread) return;

	this->thread->Join();
	this->joined_thread = true;
}

/**
* Run all jobs for the given LinkGraphJobGroup. This method is tailored to
* ThreadObject::New.
* @param group Pointer to a LinkGraphJobGroup.
*/
/* static */
void LinkGraphJobGroup::Run(void* group)
{
	auto* job_group = static_cast<LinkGraphJobGroup *>(group);

	for (LinkGraphJob* job : job_group->jobs) {
		LinkGraphSchedule::Run(job);
	}
}

/* static */
void LinkGraphJobGroup::ExecuteJobSet(std::vector<JobInfo> jobs)
{
	const uint thread_budget = 200000;

	std::sort(jobs.begin(), jobs.end(), [](const JobInfo& a, const JobInfo& b)
	{
		return a.cost_estimate < b.cost_estimate;
	});

	std::vector<LinkGraphJob *> bucket;
	uint bucket_cost = 0;

	const auto flush_bucket = [&]()
	{
		if (!bucket_cost) return;
		auto group = std::make_shared<LinkGraphJobGroup>(constructor_token(), std::move(bucket));
		group->SpawnThread();
		bucket_cost = 0;
		bucket.clear();
	};

	for (JobInfo& job_info : jobs) {
		if (bucket_cost && (bucket_cost + job_info.cost_estimate > thread_budget)) flush_bucket();
		bucket.push_back(job_info.job);
		bucket_cost += job_info.cost_estimate;
	}

	flush_bucket();
}

LinkGraphJobGroup::JobInfo::JobInfo(LinkGraphJob* job) :
	job(job),
	cost_estimate(job->Graph().CalculateCostEstimate()) { }

/**
* Pause the game if on the next _date_fract tick, we would do a join with the next
* link graph job, but it is still running.
* If we previous paused, unpause if the job is now ready to be joined with
*/
void StateGameLoop_LinkGraphPauseControl()
{
	if (_pause_mode & PM_PAUSED_LINK_GRAPH) {
		// We are paused waiting on a job, check the job every tick.
		if (!LinkGraphSchedule::instance.IsJoinWithUnfinishedJobDue()) {
			DoCommandP(0, PM_PAUSED_LINK_GRAPH, 0, CMD_PAUSE);
		}
	} else if (_pause_mode == PM_UNPAUSED) {
		// Check for zero also if we're in the main menu and haven't loaded the setting yet.
		if (_settings_game.economy.daylength <= 1) {
			if (_date_fract != LinkGraphSchedule::SPAWN_JOIN_TICK - 1) return;
			if (_date % _settings_game.linkgraph.recalc_interval != _settings_game.linkgraph.recalc_interval / 2) return;
		} else {
			const int date_ticks = ((_date * DAY_TICKS) + _date_fract - (LinkGraphSchedule::SPAWN_JOIN_TICK - 1));
			const auto interval = max<int>(2, ((_settings_game.linkgraph.recalc_interval * DAY_TICKS) / _settings_game.economy.daylength));
			if (date_ticks % interval != interval / 2) return;
		}

		// Perform check one _date_fract tick before we would join.
		if (LinkGraphSchedule::instance.IsJoinWithUnfinishedJobDue()) {
			DoCommandP(0, PM_PAUSED_LINK_GRAPH, 1, CMD_PAUSE);
		}
	}
}

/**
* Spawn or join a link graph job or compress a link graph if any link graph is
* due to do so.
*/
void OnTick_LinkGraph()
{
	int offset;
	int interval;

	// Check for zero daylength if settings are not loaded yet.
	if (_settings_game.economy.daylength <= 1) {
		if (_date_fract != LinkGraphSchedule::SPAWN_JOIN_TICK) return;
		interval = _settings_game.linkgraph.recalc_interval;
		offset = _date % interval;
	} else {
		interval = max<int>(2, _settings_game.linkgraph.recalc_interval * DAY_TICKS / _settings_game.economy.daylength);
		offset = (_date * DAY_TICKS + _date_fract - LinkGraphSchedule::SPAWN_JOIN_TICK) % interval;
	}

	if (offset == 0) {
		LinkGraphSchedule::instance.SpawnNext();
	} else if (offset == interval / 2) {
		LinkGraphSchedule::instance.JoinNext();
	}
}

