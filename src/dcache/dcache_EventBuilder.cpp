/*
 *	dcache::EventBuilder class implementation
 *
 *	This file is part of OTAWA
 *	Copyright (c) 2010, IRIT UPS.
 *
 *	OTAWA is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	OTAWA is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with OTAWA; if not, write to the Free Software
 *	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <otawa/cfg/features.h>
#include <otawa/cfg/Loop.h>
#include <otawa/dcache/features.h>
#include <otawa/events/features.h>
#include <otawa/hard/CacheConfiguration.h>
#include <otawa/hard/Machine.h>
#include <otawa/hard/Memory.h>
#include <otawa/ipet.h>

namespace otawa { namespace dcache {

class MemEvent: public Event {
public:
	MemEvent(Inst *inst, ot::time cost):
		otawa::Event(inst),
		h(nullptr),
		_cost(cost),
		_occ(NO_OCCURRENCE),
		nc(false)
		{ }
	
	ot::time cost(void) const override { return _cost; }
	type_t type(void) const override { return LOCAL; }
	occurrence_t occurrence(void) const override { return _occ; }
	int weight(void) const override { return 0; }
	kind_t kind(void) const override { return MEM; }
	cstring name(void) const override { return "ME"; }
	
	virtual string detail() const override {
		StringBuffer buf;	
		buf << "fetch @" << this->inst()->topAddress() << ' ' << _occ;
		if(_occ == SOMETIMES) {
			buf << " (";
			
			if(!ah.isEmpty()) {
				bool fst = true;
				buf << "AH from ";
				for(auto e: ah) {
					if(fst)
						fst = false;
					else
						buf << ", ";
					buf << e->source();
				}
			}
			
			if(!am.isEmpty()) {
				if(!ah.isEmpty())
					buf << ", ";
				bool fst = true;
				buf << "AM from ";
				for(auto e: am) {
					if(fst)
						fst = false;
					else
						buf << ", ";
					buf << e->source();
				}
			}
			
			if(h != nullptr) {
				if(!am.isEmpty() || !ah.isEmpty())
					buf << ", ";
				buf << "PE at " << h;
			}

			buf << ")";
		}
		return buf.toString();
	}
	
	bool isEstimating(bool on) override { return (on && !nc) && (!on && !ah.isEmpty()); }
	
	void estimate(ilp::Constraint *cons, bool on) override {
		if(on) {
			// ... <= +oo
			if(nc)
				cons->addRight(type_info<double>::max, nullptr);
			
			// ... <= sum of AM count edge and of non-back entering h count
			else {
				for(auto e: am)
					cons->addRight(1, ipet::VAR(e));
				if(h != nullptr)
					for(auto e: h->inEdges())
						if(!otawa::BACK_EDGE(e))
							cons->addRight(1, ipet::VAR(e));				
			}
		}
		else
			// ... >= sum of AH edge count
			for(auto e: ah)
				cons->addRight(1, ipet::VAR(e));
	}

	void account(Edge *e, const dcache::BlockAccess& acc) {
		switch(dcache::CATEGORY(acc)) {
		
		case cache::ALWAYS_HIT:
			switch(_occ) {
			case Event::NO_OCCURRENCE:
				_occ = NEVER;
				break;
			case Event::ALWAYS:
			case Event::SOMETIMES:
				_occ = SOMETIMES;
				break;
			case Event::NEVER:
				break;
			};
			ah.add(e);
			break;
		
		case cache::ALWAYS_MISS:
			switch(_occ) {
			case Event::NO_OCCURRENCE:
				_occ = ALWAYS;
				break;
			case Event::NEVER:
			case Event::SOMETIMES:
				_occ = SOMETIMES;
				break;
			case Event::ALWAYS:
				break;
			};
			am.add(e);
			break;			
		
		case cache::FIRST_MISS:
			_occ = Event::SOMETIMES;
			if(h == nullptr)
				h = *dcache::CATEGORY_HEADER(e);
			else {
				auto nh = dcache::CATEGORY_HEADER(e);
				if(Loop::of(h)->includes(Loop::of(nh)))
					h = nh;
			}
			break;
		
		case cache::NOT_CLASSIFIED:
		case cache::FIRST_HIT:
			_occ = Event::SOMETIMES;
			nc = true;
			break;
		
		case cache::INVALID_CATEGORY:
		case cache::TOP_CATEGORY:
			ASSERTP(false, _ << "invalid dcache category: %d" << dcache::CATEGORY(acc));
			break;
		}
	}

private:
	List<Edge *> ah, am;
	otawa::Block *h;
	ot::time _cost;
	occurrence_t _occ;
	bool nc;
};


/**
 * Build events of data cache accesses for a block.
 * 
 * **Provided features:**
 * * otawa::dcache::EVENTS_FEATURE
 * 
 * **Required features:**
 * * otawa::dcache::CATEGORY_FEATURE
 * * otawa::hard::MACHINE_FEATURE
 * 
 * @ingroup events
 */
class EventBuilder: public BBProcessor {
public:
	static p::declare reg;
	EventBuilder(p::declare& r = reg): BBProcessor(r), mach(nullptr) { }

protected:

	///
	void setup(WorkSpace *ws) override {
		mach = hard::MACHINE_FEATURE.get(ws);
	}

	///
	void processBB(WorkSpace *ws, CFG *g, otawa::Block *b) override {
		if(!b->isBasic())
			return;
		auto bb = b->toBasic();

		Vector<MemEvent *> evts;
		
		// build events
		{
			for(const auto& a: *DATA_BLOCKS(*bb->inEdges().begin())) {
				MemEvent *evt = nullptr;
				if(a.action() == dcache::BlockAccess::PURGE)
					continue;
				
				// build the event
				// TODO Taker into account case where address is different according to predecessors
				ot::time t;
				Address addr = a.address();
				if(addr.isNull()) {
					// TODO should be a range
					if(a.action() == dcache::BlockAccess::LOAD)
						t = mach->memory->worstReadAccess();
					else
						t = mach->memory->worstWriteAccess();				
				}
				else {
					if(a.action() == dcache::BlockAccess::LOAD)
						t = mach->memory->readTime(addr);
					else
						t = mach->memory->writeTime(addr);
				}
				evt = new MemEvent(a.instruction(), t);
				evts.add(evt);
				EVENT(bb).add(evt);
			}			
		}
		
		// record categories
		for(auto e: bb->inEdges()) {
			int i = 0;
			auto& as = *DATA_BLOCKS(*bb->inEdges().begin());
			for(int j = 0; j < as.count(); j++) {
				const auto& a = as[j];
				if(a.action() == BlockAccess::PURGE)
					continue;
				evts[i]->account(e, a);
				i++;
			}
		}
	}

	///
	void dumpBB(otawa::Block *b, io::Output& out) override {
		for(auto e: EVENT.all(b)) {
			auto evt = dynamic_cast<MemEvent *>(e);
			if(evt != nullptr)
				out << "\t\t" << e->detail() << io::endl;
		}
	}

private:
	const hard::Machine *mach;
};


///
p::declare EventBuilder::reg = p::init("otawa::dcache::EventBuilder", Version(1, 0, 0))
	.make<EventBuilder>()
	.extend<BBProcessor>()
	.provide(EVENTS_FEATURE)
	.require(hard::MACHINE_FEATURE)
	.require(LOOP_INFO_FEATURE)
	.require(CATEGORY_FEATURE);

///

p::feature EVENTS_FEATURE("otawa::dcache::EVENTS_FEATURE", p::make<EventBuilder>());

} }	// otawa::dcache

