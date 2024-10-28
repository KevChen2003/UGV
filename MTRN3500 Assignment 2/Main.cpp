#include "TMM.h"

int main(void) {
	ThreadManagement^ tmm = gcnew ThreadManagement();

	tmm->setupSharedMemory();
	tmm->threadFunction();
	return 0;
}