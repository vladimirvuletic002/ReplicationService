
#include"ReplicationHelper.h"
int errorCounter = 0;


void printfP(const char* text, Measurement* m, std::atomic<int> totalMessages,std::atomic<int> totalBackup) {
    if (m != nullptr) {
        std::cout << std::left << "\n\n---\t[ Original server ]---[\t" << text << "\t]----------------" <<
            "\n---\t\tData recieved from process [ " << m->deviceId << " ]\t-------------------------\n" << std::endl;

        printMeasurement(m);

        std::cout << "\n-------------------------______________----------_________-----------_______-----__-_.." << std::endl;
    }
    else {
        std::cout << "\n\t[ Original server ]------------------------------------------------------|" <<
            "\n\t\t|>>>>>\t\t" << text << "\t\t<<<<<|" <<
            "\n--------->  Total messages still in backup: "<<totalBackup<<"\t\t<<<<|"<<
            "\n--------->  Total messages confirmed by the server: "<<totalMessages<<"\t\t<<<<|"<<
            "\n______-----_________--------------__________---------_______-------______-----__--__--__-...\t\n" << std::endl;

    }
}




void ReceiveAndFilterMessages(std::atomic<bool>& stopFlag, Measurement& m, SOCKET& backupRequest, std::atomic<int>& totalBackup,
    bool& stop,int& localCounter, unordered_map &messageQueue, std::mutex& mtx)
{   

    switch (ReceiveWithNonBlockingSocket(stopFlag, backupRequest, m)) {

    case END:
        //stopFlag = true;
        stop = true;
        break;

    case SKIP:
        break;

    case PROCEED:

        if (m.purpose == END_OF_QUEUE) {
            printf("\n\t\t\t\tBackup finished.\n");
            stop = true;
            m.purpose = 0;
            break;
        }
        if (m.purpose == EMPTY_QUEUE) {
            printf("\n\t\t\t\tNo backup for this ID");
            stop = true;
            m.purpose = 0;
            break;
        }
        if (m.purpose == -4) {
            printf("\n\t\t\tThere is no process assigned to such ID...");
            stop = true;
            m.purpose = 0;
            break;
        }//if (m.purpose == 55) {
        
     
        if (EnqueueToMap(mtx, messageQueue, m)) {
            localCounter++;
            totalBackup++;
        }
        
        if (m.purpose != 8 && localCounter < 100) {//Dont printf if it's mass send or more than 100 messages already passed for specific ID
            printfP("Backup", &m,0,0);
        }


    }
}