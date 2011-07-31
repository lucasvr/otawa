/*
 * test_newexegraphs.cpp
 *
 *  Created on: 29 juil. 2011
 *      Author: rochange
 */


#include <otawa/app/Application.h>
#include <otawa/prog/Manager.h>
#include <otawa/cfg/features.h>
//#include <elm/system/System.h>
#include "EGBBTime.h"
#include "ExecutionGraph.h"
#include "EGBlockSeqList.h"

using namespace otawa;
using namespace elm;

class MyApp: public Application {

public:

	MyApp(void)
	:	Application("test_newexegraphs"),
		graphs(*this, 'g', "graphs", "directory to dump execution graphs", "PATH", "")
	{ }

	~MyApp(void) {
	}

protected:

	virtual void work(const String &entry, PropList &props) throw (elm::Exception) {
		WorkSpace *ws = workspace();
		cout << "Processing function: " << (TASK_ENTRY(props)) << "\n";

		// build execution graphs
		newexegraph::EGBlockSeqListFactory block_seq_list_factory;
		newexegraph::EGBBTime<newexegraph::ExecutionGraph> bbtime(&block_seq_list_factory, props);
		bbtime.process(ws, props);
	}

	virtual void prepare(PropList &props) {
		PROCESSOR_PATH(props) = "pipeline.xml";
		if(graphs)
			newexegraph::GRAPHS_DIR(props) = graphs.value();
	}

private:
	option::StringOption graphs;
};


int main(int argc, char **argv) {
	MyApp app;
	return app.run(argc, argv);
}

