#include "RelOp.h"

// SelectPipe

void *SelectPipe::selectPipe(void *arg) {
	SelectPipe *sp = (SelectPipe *) arg;
	sp->DoSelectPipe();
	return NULL;
}

void SelectPipe::DoSelectPipe() {
	Record *tmpRecord = new Record();
	ComparisonEngine cmp;
	while(this->inPipe->Remove(tmpRecord)) {
		if(cmp.Compare(tmpRecord, this->literal, this->selOp)) {	// compares record with CNF, if true puts it into outPipe
			this->outPipe->Insert(tmpRecord);
		}
	}
	delete tmpRecord;
	this->outPipe->ShutDown();
}

void SelectPipe::Run (Pipe &inPipe, Pipe &outPipe, CNF &selOp, Record &literal){
	this->inPipe = &inPipe;
	this->outPipe = &outPipe;
	this->selOp = &selOp;
	this->literal = &literal;
	pthread_create(&thread, NULL, selectPipe, this);
}

void SelectPipe::WaitUntilDone () {
	pthread_join (thread, NULL);
}

void SelectPipe::Use_n_Pages (int runlen) {
	this->nPages = runlen;
}

// SelectFile

void *SelectFile::selectFile(void *arg) {
	SelectFile *sp = (SelectFile *) arg;
	sp->DoSelectFile();
	return NULL;
}

void SelectFile::DoSelectFile() {
	Record *tmpRecord = new Record();
	ComparisonEngine cmp;
	while(this->inFile->GetNext(*tmpRecord)) {
		if(cmp.Compare(tmpRecord, this->literal, this->selOp)) {	// compares record with CNF, if true puts it into outPipe
			this->outPipe->Insert(tmpRecord);
		}
	}
	delete tmpRecord;
	this->outPipe->ShutDown();
}

void SelectFile::Run (DBFile &inFile, Pipe &outPipe, CNF &selOp, Record &literal) {
	this->inFile = &inFile;
	this->outPipe = &outPipe;
	this->selOp = &selOp;
	this->literal = &literal;
	pthread_create(&thread, NULL, selectFile, this);
}

void SelectFile::WaitUntilDone () {
	pthread_join (thread, NULL);
}

void SelectFile::Use_n_Pages (int runlen) {
	this->nPages = runlen;
}


// Project 

void *Project::project(void *arg) {
	Project *pj = (Project *) arg;
	pj->DoProject();
	return NULL;
}

void Project::DoProject() {
	Record *tmpRcd = new Record;
	while(this->inPipe->Remove(tmpRcd)) {
		tmpRcd->Project(this->keepMe, this->numAttsOutput, this->numAttsInput);
		this->outPipe->Insert(tmpRcd);
	}
	this->outPipe->ShutDown();
	delete tmpRcd;
	return;
}

void Project::Run (Pipe &inPipe, Pipe &outPipe, int *keepMe,
		int numAttsInput, int numAttsOutput) {
	this->inPipe = &inPipe;
	this->outPipe = &outPipe;
	this->keepMe = keepMe;
	this->numAttsInput = numAttsInput;
	this->numAttsOutput = numAttsOutput;
	pthread_create(&thread, NULL, project, this);
}

void Project::WaitUntilDone () {
	pthread_join (thread, NULL);
}

void Project::Use_n_Pages (int n) {
	this->nPages = n;
}

// Duplicate Removal

void *DuplicateRemoval::duprem(void *arg){
	DuplicateRemoval *dr = (DuplicateRemoval *) arg;
	dr->DoDuplicateRemoval();	
	return NULL;
}

void DuplicateRemoval::DoDuplicateRemoval(){

	OrderMaker *om = new OrderMaker(this->mySchema);
	cout <<"duplicate removal on ordermaker: "; om->Print();
//	Attribute *atts = this->mySchema->GetAtts();
		// loop through all of the attributes, and list as it is

//	cout <<"distince on ";
//	om->Print(); cout <<endl;
	if(!om)
		cerr <<"Can not allocate ordermaker!"<<endl;
	Pipe sortPipe(1000);
//	int runlen = 50;//temp...
	BigQ *bigQ = new BigQ(*(this->inPipe), sortPipe, *om, nPages);

	ComparisonEngine cmp;
	Record *tmp = new Record();
	Record *chk = new Record();
	if(sortPipe.Remove(tmp)) {
		//insert the first one
		bool more = true;
		while(more) {
			more = false;
			Record *copyMe = new Record();
			copyMe->Copy(tmp);
			this->outPipe->Insert(copyMe);
			while(sortPipe.Remove(chk)) {
				if(cmp.Compare(tmp, chk, om) != 0) { //equal
					tmp->Copy(chk);
					more = true;
					break;
				}
			}
		}
	}

//	sortPipe.ShutDown();
	this->outPipe->ShutDown();
}

void DuplicateRemoval::Run(Pipe &inPipe, Pipe &outPipe, Schema &mySchema) { 
	this->inPipe = &inPipe;
	this->outPipe = &outPipe;
	this->mySchema = &mySchema;
	pthread_create(&thread,NULL,duprem,this);
}

void DuplicateRemoval::WaitUntilDone () { 
	pthread_join(thread,NULL);
}

void DuplicateRemoval::Use_n_Pages (int n) { 
	this->nPages = n;
}


/*class Join : public RelationalOp { 
	public:
	void Run (Pipe &inPipeL, Pipe &inPipeR, Pipe &outPipe, CNF &selOp, Record &literal) { }
	void WaitUntilDone () { }
	void Use_n_Pages (int n) { }
};*/

void Join::Run (Pipe &inPipeL, Pipe &inPipeR, Pipe &outPipe, CNF &selOp, Record &literal) { 

	this->inPipeL = &inPipeL;
	this->inPipeR = &inPipeR;
	this->outPipe = &outPipe;
	this->selOp = &selOp;
	this->literal = &literal;
	pthread_create(&thread,NULL,jswpn,this);

}

void Join::jspwn(void arg){

	Join *j = (Join *) arg;
	j->join();	

}


void Join::join(){

	/*OrderMaker *omL = new OrderMaker();
	OrderMaker *omR = new OrderMaker();


	if(selOp->GetSortOrders(*omL,*omR)==0){}
	else{
		
		Pipe *OL = new Pipe(100);
		Pipe *OR = new Pipe(100);

		BigQ L = BigQ (*inPipeL, *OL, *omL, nPages);
		BigQ R = BigQ (*inPipeR, *OR, *omR, nPages);

		Record *RL = new Record();
		Record *RR = new Record();

		int resultL = OL->Remove(RL);
		int resultR = OR->Remove(RR);

		while(resultL&&resultR){

			

		}
	

	}

	*/


        Pipe *LO = new Pipe(100);
        Pipe *RO = new Pipe(100);

        OrderMaker *omL = new OrderMaker();
        OrderMaker *omR = new OrderMaker();
    
	ComparisonEngine compEng;

        int sortMergeFlag = selOP->GetSortOrders(*omL, *omR);

        // if sort merge flag != 0 perform SORT-MERGE JOIN
        if (sortMergeFlag != 0) {

                // sort left pipe
                BigQ L(*inPipeL, *LO, *omL, params->runLength);

                // sort right pipe
                BigQ R(*inPipeR, *RO, *omR, params->runLength);

                Record RL;
                Record *RR = new Record();
                vector<Record*> mergeVector; // to store records with same value of join attribute

                int isLeftPipeEmpty = LO->Remove(&RL);
                int isRightPipeEmpty = RO->Remove(RR);

                int numLeftAttrs = RL.getNumAttrs();
                int numRightAttrs = RR->getNumAttrs();

                // array used as input to merge record function
                int attrsToKeep[numLeftAttrs + numRightAttrs];
             
	        int k = 0;
             
		for (int i = 0; i < numLeftAttrs; i++) {
                        attrsToKeep[k++] = i;
                }

                for (int i = 0; i < numRightAttrs; i++) {
                        attrsToKeep[k++] = i;
                }

                Record mergedRecord;
                int mergedRecCount = 0;

                // get records from left output pipe and right output pipe
                while (isLeftPipeEmpty != 0 && isRightPipeEmpty != 0) {
                        
                        int orderMakerAnswer = compEng.Compare(&RL, omL, RR, omR);
                        
                        // if left and right record are equal on join attributes
                        if (orderMakerAnswer == 0) {
     
                                for (int i = 0; i < mergeVector.size(); i++) {
                                        delete mergeVector[i];
                                        mergeVector[i] = NULL;
                                }
                                mergeVector.clear();
                                
                                // get all matching records (on join attr) in vector
                                while (orderMakerAnswer == 0 && isRightPipeEmpty != 0) {
                                        mergeVector.push_back(rightRec);
                                        RR = new Record();
                                        isRightPipeEmpty = RO->Remove(RR);
                                        if (isRightPipeEmpty != 0) {
                                                orderMakerAnswer = compEng.Compare(&RL, omL, RR, omR);
                                        }
                                }

                                // compare left Rec with first from vector 
                                orderMakerAnswer = compEng.Compare(&RL, omL, mergeVector[0], omR);
                                
                                // compare left Rec with all records from vector
                                while (orderMakerAnswer == 0 && isLeftPipeEmpty != 0) {

                                        for (int i = 0; i < mergeVector.size(); i++) {
                                                mergedRecord.MergeRecords(&RL, mergeVector[i], numLeftAttrs, numRightAttrs, attrsToKeep, numLeftAttrs + numRightAttrs, numLeftAttrs);
                                                outPipe->Insert(&mergedRecord);
                                                mergedRecCount++;
                                        }
                                        
                                        //Take next Record from left pipe;
                                        isLeftPipeEmpty = LO->Remove(&leftRec);
                                        orderMakerAnswer = compEng.Compare(&RL, omL, mergeVector[0], omR);

                                }

                        } 
                        // take next from left pipe if it is smaller , else take next from right pipe
                        else if (orderMakerAnswer < 0) {
                                isLeftPipeEmpty = LO->Remove(&RL);                                
                        } 
                        else {
                                isRightPipeEmpty = RO->Remove(RR);
                        }
                }
                cout << "Total Records Merged :: " << mergedRecCount << endl;

        }
        // BLOCK NESTED LOOP JOIN - if no suitable order maker found
        else {
                
                int mergedRecCount = 0;
                char fileName[200];
                int x = rand();
                sprintf(fileName, "rightRelation_bnj.tmp%d", x);

                DBFile rightRelationFIle;
                Record rightRecord;
                rightRelationFIle.Create(fileName, heap, NULL);

                //Add entire right relation to a temporary dbFile
                while (inPipeR->Remove(&RR)) {
                        rightRelationFIle.Add(RR);
                }
                rightRelationFIle.Close();

                vector<Record*> leftRelationVector;
                int numPagesAllocatedForLeft = 0;
                Page pageForLeftRecords;
                Record leftRecord;
                Record* rec = new Record();

                // Remove records from left pipe in units of block ( block is n - 1 pages)                
                int isLeftRecordPresent = inPipeL->Remove(&leftRecord);
                bool isPipeEnded = false;
                int isRecordAdded;
                
                while (isLeftRecordPresent || isPipeEnded) {
                        
                        // Start reading left Record into a page
                        if (isLeftRecordPresent) {
                                isRecordAdded = pageForLeftRecords.Append(&leftRecord);
                        }
                        // when page is full or pipe is empty
                        if (isRecordAdded == 0 || isPipeEnded) {

                                // increment number of pages used
                                numPagesAllocatedForLeft++;
                                
                                // flush records of the page into vector
                                while (pageForLeftRecords.GetFirst(rec)) {
                                        leftRelationVector.push_back(rec);
                                        rec = new Record();
                                }
                                
                                //For block nested loop join, n-1 pages of left relation are joined with each record of right relation
                                
                                // start reading right relation from file when n - 1 buffer pages are full OR pipe is empty
                                if (numPagesAllocatedForLeft == params->runLength - 1 || isPipeEnded) {

                                        rightRelationFIle.Open(fileName);
                                        rightRelationFIle.MoveFirst();
                                        Record rightRec;
                                        int isRightRecPresent = rightRelationFIle.GetNext(rightRec);
                                        
                                        Record mergedRecord;
                                        int numLeftAttrs = leftRelationVector[0]->getNumAttrs();
                                        int numRightAttrs = rightRec.getNumAttrs();
                                        int attrsToKeep[numLeftAttrs + numRightAttrs];
                                        int k = 0;
                                        for (int i = 0; i < numLeftAttrs; i++) {
                                                attrsToKeep[k++] = i;
                                        }
                                        for (int i = 0; i < numRightAttrs; i++) {
                                                attrsToKeep[k++] = i;
                                        }

                                        // while right relation file has next record
                                        while (isRightRecPresent != 0) {
                                                for (int i = 0; i < leftRelationVector.size(); i++) {
                                                        
                                                        int isAccepted = compEng.Compare(leftRelationVector[i], &rightRec, literal, selOP);
                                                       
                                                        // merge records when the cnf is accepted
                                                        if (isAccepted != 0) {
                                                                mergedRecord.MergeRecords(leftRelationVector[i], &rightRec, numLeftAttrs, numRightAttrs, attrsToKeep, numLeftAttrs + numRightAttrs, numLeftAttrs);
                                                                outPipe->Insert(&mergedRecord);
                                                                mergedRecCount++;
                                                        }
                                                }
                                                isRightRecPresent = rightRelationFIle.GetNext(rightRec);
                                        }
                                        rightRelationFIle.Close();

                                        // flush the vector 
                                        numPagesAllocatedForLeft = 0;
                                        for (int i = 0; i < leftRelationVector.size(); i++) {
                                                delete leftRelationVector[i];
                                                leftRelationVector[i] = NULL;
                                        }
                                        leftRelationVector.clear();
                                        
                                        // exit loop is pipe is empty
                                        if (isPipeEnded)
                                                break;
                                }

                        }
                        
                        isLeftRecordPresent = inPipeL->Remove(&leftRecord);
                        if (isLeftRecordPresent == 0) {
                                isPipeEnded = true;
                        }
                }
                cout << "Total Records Merged :: " << mergedRecCount << endl;
                remove(fileName);
        }
        
        // shut down output pipe after join is complete
        outPipe->ShutDown();


}

void Join::WaitUntilDone () { 

	pthread_join(thread,NULL);

}


void Join::Use_n_Pages (int n) { 

	this->nPages = n;

}


void* doSum() {

        cout << "Starting Summation" << endl;

        //struct RunParams* params = (struct RunParams*) parameters;

        int intSum = 0;
        double doubleSum = 0.0;
        int intAttrVal = 0;
        double doubleAttrVal = 0.0;
        Record rec;
        Function *function = function;
        Type type;

        while (inPipe->Remove(&rec)) {

                // get value and type of the particular attribute to be summed
                type = function->Apply(rec, intAttrVal, doubleAttrVal);

                if (type == Int) {
                        intSum += intAttrVal;

                } else {
                        doubleSum += doubleAttrVal;
                }
        }



        ostringstream result;
        string resultSum;
        Record resultRec;

        // create output record 
        if (type == Int) {

                result << intSum;
                resultSum = result.str();
                resultSum.append("|");

                Attribute IA = {"int", Int};
                Schema out_sch("out_sch", 1, &IA);
                resultRec.ComposeRecord(&out_sch, resultSum.c_str());

        } else {

                result << doubleSum;
                resultSum = result.str();
                resultSum.append("|");

                Attribute DA = {"double", Double};
                Schema out_sch("out_sch", 1, &DA);
                resultRec.ComposeRecord(&out_sch, resultSum.c_str());
        }

        outPipe->Insert(&resultRec);
        outPipe->ShutDown();

}



void Sum::sspwn(void arg){

	Sum *s = (Sum *) arg;
	s->doSum();	

}

void Sum::Run(Pipe &inPipe, Pipe &outPipe, Function &computeMe) {

        cout << "In Summation Run" << endl;
        //struct RunParams* params = (RunParams*) malloc(sizeof (RunParams));
        this->inPipe = &inPipe;
        this->outPipe = &outPipe;
        this->function = &computeMe;

        pthread_create(&thread, NULL, sspwn, this);
}

void Sum::WaitUntilDone() {

        pthread_join(thread, NULL);
        cout << "\n Summation Complete" << endl;

}

void Sum::Use_n_Pages(int n) {

        cout << "Setting run length in use n pages : " << n << endl;
        this->nPages = n;
}



void* doGroupBy() {

        cout << "Starting group by" << endl;
        struct RunParams* params = (struct RunParams*) parameters;

        Attribute DA = {"double", Double};
        Schema out_sch_double("out_sch", 1, &DA);
        
        Attribute IA = {"int", Int};
        Schema out_sch_int("out_sch", 1, &IA);

        vector<int> groupByOrderAttrs;
        vector<Type> groupByOrderTypes;
        params->groupbyOrder->GetOrderMakerAttrs(&groupByOrderAttrs, &groupByOrderTypes);

        int *projectAttrsToKeep = &groupByOrderAttrs[0];

        int numGroupByAttrs = groupByOrderAttrs.size();

        int mergeAttrsToKeep[1 + numGroupByAttrs];
        mergeAttrsToKeep[0] = 0;
        for (int i = 1; i <= numGroupByAttrs; i++) {
                mergeAttrsToKeep[i] = i - 1;
        }

        Pipe *bigqOutPipe = new Pipe(100);
        BigQ bigQ(*params->inputPipe, *bigqOutPipe, *params->groupbyOrder, params->runLength);

        ComparisonEngine comparator;
        int intAttrVal;
        double doubleAttrVal;
        Type type;
        int intSum = 0;
        double doubleSum = 0;
        Record currRec, nextRec;

        int currRecPresent = bigqOutPipe->Remove(&currRec);
        int nextRecPresent = currRecPresent;
        nextRec.Copy(&currRec);
        int numCurrRecAttrs = currRec.getNumAttrs();

        while (nextRecPresent) {

                int orderMakerAnswer = comparator.Compare(&currRec, &nextRec, params->groupbyOrder);
                
                // Perform summation until there is a mismatch
                if (orderMakerAnswer == 0) {

                        type = params->function->Apply(nextRec, intAttrVal, doubleAttrVal);

                        if (type == Int) {
                                intSum += intAttrVal;

                        } else {
                                doubleSum += doubleAttrVal;
                        }

                        nextRecPresent = bigqOutPipe->Remove(&nextRec);

                } 
                // create output tuple when there is a mismatch
                else {

                        ostringstream result;
                        string resultSum;
                        Record groupSumRec;

                        // create output record 
                        if (type == Int) {

                                result << intSum;
                                resultSum = result.str();
                                resultSum.append("|");
                                groupSumRec.ComposeRecord(&out_sch_int, resultSum.c_str());

                        } else {

                                result << doubleSum;
                                resultSum = result.str();
                                resultSum.append("|");
                                groupSumRec.ComposeRecord(&out_sch_double, resultSum.c_str());
                        }

                        // Get a record contaning only group by attributes
                        currRec.Project(projectAttrsToKeep, numGroupByAttrs, numCurrRecAttrs);

                        // merge the above record with the group Sum Record to get result record
                        Record resultRec;
                        resultRec.MergeRecords(&groupSumRec, &currRec, 1, numGroupByAttrs, mergeAttrsToKeep, 1 + numGroupByAttrs, 1);

                        params->outputPipe->Insert(&resultRec);

                        currRec.Copy(&nextRec);
                        intSum = 0;
                        doubleSum = 0;
                }
        }

        ostringstream result;
        string resultSum;
        Record groupSumRec;
        
        if(type == Int){
                 
                 result << intSum;
                 resultSum = result.str();
                 resultSum.append("|");
                 groupSumRec.ComposeRecord(&out_sch_int, resultSum.c_str());
                
        }
        else{
                
                result << doubleSum;
                resultSum = result.str();
                resultSum.append("|");
                groupSumRec.ComposeRecord(&out_sch_double, resultSum.c_str());         
        }
        
        // Get a record containing only group by attributes
        currRec.Project(projectAttrsToKeep, numGroupByAttrs, numCurrRecAttrs);

        // merge the above record with the group Sum Record to get result record
        Record resultRec;
        resultRec.MergeRecords(&groupSumRec, &currRec, 1, numGroupByAttrs, mergeAttrsToKeep, 1 + numGroupByAttrs, 1);

        params->outputPipe->Insert(&resultRec);
        params->outputPipe->ShutDown();
}

void GroupBy::gspwn(void arg){

	GroupBy *g = (GroupBy *) arg;
	g->doGroupBy();	

}

void GroupBy::Run(Pipe &inPipe, Pipe &outPipe, OrderMaker &groupAtts, Function &computeMe) {

        cout << "In Group By Run" << endl;

        this->inPipe = &inPipe;
        this->outPipe = &outPipe;
        this->groupbyOrder = &groupAtts;
        this->function = &computeMe;
        this->runLength = runLength;

        pthread_create(&thread, NULL, gspwn, this);
}

void GroupBy::WaitUntilDone() {

        pthread_join(thread, NULL);
        cout << "\n Group By Complete" << endl;

}

void GroupBy::Use_n_Pages(int n) {

        cout << "Setting run length in use n pages : " << n << endl;
        this->nPages = n;
}
