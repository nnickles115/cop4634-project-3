/***************************************************************/
/*                                                             */
/* lizards.cpp                                                 */
/*                                                             */
/* To compile, you need all the files listed below             */
/*   lizards.cpp                                               */
/*                                                             */
/* Be sure to use the -lpthread option for the compile command */
/*   g++ -g -Wall -std=c++11 lizard.cpp -o lizard -lpthread    */
/*                                                             */
/* Execute with the -d command-line option to enable debugging */
/* output.  For example,                                       */
/*   ./lizard -d                                               */
/*                                                             */
/***************************************************************/
// C Includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>

// C++ Inlcudes
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

// Usings
using namespace std;

// Define "constant" values here
/*
 * Make this 1 to check for lizards travelling in both directions
 * Leave it 0 to allow bidirectional travel
 */
#define UNIDIRECTIONAL       0

/*
 * Set this to the number of seconds you want the lizard world to
 * be simulated.  
 * Try 30 for development and 120 for more thorough testing.
 */
#define WORLDEND             30

// Number of lizard threads to create
#define NUM_LIZARDS          20

// Number of cat threads to create
#define NUM_CATS             2

// Maximum lizards crossing at once before alerting cats
#define MAX_LIZARD_CROSSING  4

// Maximum seconds for a lizard to sleep
#define MAX_LIZARD_SLEEP     3

// Maximum seconds for a cat to sleep
#define MAX_CAT_SLEEP        3

// Maximum seconds for a lizard to eat
#define MAX_LIZARD_EAT       5

// Number of seconds it takes to cross the driveway
#define CROSS_SECONDS        2

// Declare global variables here
mutex cout_mutex; // Ensure debug output is not being overwritten
mutex crossing_mutex; // Ensure counters avoid race conditions
sem_t driveway_sem; // Semaphore to control num of lizards on the driveway

/**************************************************/
/* Please leave these variables alone.  They are  */
/* used to check the proper functioning of your   */
/* program.  They should only be used in the code */
/* I have provided.                               */
/**************************************************/
int numCrossingSago2MonkeyGrass;
int numCrossingMonkeyGrass2Sago;
int debug;
int running;
/**************************************************/

/**
 * This class models a cat that sleep, wakes-up, checks on lizards in the driveway
 * and goes back to sleep. If the cat sees enough lizards it "plays" with them.
 */
class Cat {
	int     _id;        // the Id of the cat
	thread* _catThread; // the thread simulating the cat
	
	public:
		Cat(int id);
		int getId();
		void run();
		void wait();
    
  private:
		void sleepNow();
    static void catThread (Cat *aCat); 
};

/**
 * Constructs a cat.
 *
 * @param id - the Id of the cat 
 */
Cat::Cat (int id) {
	_id = id;
}

/**
 * Returns the Id of the cat.
 *
 * @return the Id of a cat
 */
int Cat::getId() {
	return _id;
}

/**
 * Launches a cat thread.
 */
void Cat::run() {
  // NN DS
  if(!_catThread) {
    _catThread = new thread(catThread, this);
  }
}

/**
 * Waits for a cat to finish running.
 */
void Cat::wait() {
  // NN DS
  if(_catThread && _catThread->joinable()) {
    _catThread->join(); // Wait for the thread to terminate
  }
}

/**
 * Simulate a cat sleeping for a random amount of time
 */
void Cat::sleepNow() {
	int sleepSeconds;

	sleepSeconds = 1 + (int)(random() / (double)RAND_MAX * MAX_CAT_SLEEP);

	if(debug) {
    lock_guard<mutex> lock(cout_mutex); // NN DS
		cout << "[" << _id << "] cat sleeping for " << sleepSeconds << " seconds" << endl;
		cout << flush;
  }

	sleep(sleepSeconds);

	if(debug) {
    lock_guard<mutex> lock(cout_mutex); // NN DS
		cout << "[" << _id << "] cat awake" << endl;
		cout << flush;
  }
}

/**
 * This simulates a cat that is sleeping and occasionally checking on
 * the driveway on lizards.
 * 
 * @param aCat - a cat that is being run concurrently
 */
void Cat::catThread(Cat *aCat) {
	if(debug) {
    lock_guard<mutex> lock(cout_mutex); // NN DS
		cout << "[" << aCat->getId() << "] cat is alive\n";
		cout << flush;
  }

	while(running) {
		aCat->sleepNow();

		// Check for too many lizards crossing
    int totalCrossing = numCrossingSago2MonkeyGrass + numCrossingMonkeyGrass2Sago; // NN DS
		if(totalCrossing > MAX_LIZARD_CROSSING) {
      lock_guard<mutex> lock(cout_mutex); // NN DS
		  cout << "\tThe cats are happy - they have toys.\n";
      exit(-1);
		}
  }
}

/**
 * This class models a lizard that sleeps, wakes-up, checks if it is safe to cross,
 * crosses over and eats, then checks if it is safe to return, and goes back to sleep.
 */
class Lizard {
	int     _id;      // the Id of the lizard
	thread* _aLizard; // the thread simulating the lizard
	
  public:
		Lizard(int id);
		int getId();
    void run();
    void wait();

  private:
		void sago2MonkeyGrassIsSafe();
		void crossSago2MonkeyGrass();
		void madeIt2MonkeyGrass();
		void eat();
		void monkeyGrass2SagoIsSafe();
		void crossMonkeyGrass2Sago();
		void madeIt2Sago();
		void sleepNow();
    static void lizardThread(Lizard *aLizard);
};

/**
 * Constructs a lizard.
 *
 * @param id - the Id of the lizard 
 */
Lizard::Lizard(int id) {
	_id = id;
}

/**
 * Returns the Id of the lizard.
 *
 * @return the Id of a lizard
 */
int Lizard::getId() {
	return _id;
}

/**
 * Launches a lizard thread.
 */
void Lizard::run() {
  // NN DS
  if(!_aLizard) {
    _aLizard = new thread(lizardThread, this);
  }
}
 
/**
 * Waits for a lizard to finish running.
 */
void Lizard::wait() {
  // NN DS
	if(_aLizard && _aLizard->joinable()) {
    _aLizard->join(); // wait for the thread to terminate
  } 
}

/**
 * Simulate a lizard sleeping for a random amount of time
 */
void Lizard::sleepNow() {
	int sleepSeconds;

	sleepSeconds = 1 + (int)(random() / (double)RAND_MAX * MAX_LIZARD_SLEEP);

	if(debug) {
    lock_guard<mutex> lock(cout_mutex); // NN DS
    cout << "[" << _id << "] sleeping for " << sleepSeconds << " seconds" << endl;
    cout << flush;
  }

	sleep(sleepSeconds);

	if(debug) {
    lock_guard<mutex> lock(cout_mutex); // NN DS
    cout << "[" << _id << "] awake" << endl;
    cout << flush;
  }
}

/**
 * Returns when it is safe for this lizard to cross from the sago
 * to the monkey grass.
 */
void Lizard::sago2MonkeyGrassIsSafe() {
	if(debug) {
    lock_guard<mutex> lock(cout_mutex); // NN DS
		cout << "[" << _id << "] checking sago -> monkey grass" << endl;
		cout << flush;
  }

  // Wait for a spot on the driveway
  sem_wait(&driveway_sem); // NN DS

	if(debug) {
    lock_guard<mutex> lock(cout_mutex); // NN DS
		cout << "[" << _id << "] thinks sago -> monkey grass is safe" << endl;
		cout << flush;
  }
}

/**
 * Delays for 1 second to simulate crossing from the sago to
 * the monkey grass. 
 */
void Lizard::crossSago2MonkeyGrass() {
	if(debug) {
    lock_guard<mutex> lock(cout_mutex); // NN DS
    cout << "[" << _id << "] crossing  sago -> monkey grass" << endl;
    cout << flush;
  }

	// One more crossing this way
	{ // NN DS
    lock_guard<mutex> lock(crossing_mutex);
    numCrossingSago2MonkeyGrass++;
  }

  // NN DS
  if(debug) {
    lock_guard<mutex> lock(cout_mutex);
		cout << numCrossingSago2MonkeyGrass << " crossing sago -> monkey grass" << endl;
    cout << flush;
  }

	// Check for lizards cross both ways
	if(numCrossingMonkeyGrass2Sago && UNIDIRECTIONAL) {
		cout << "\tCrash!  We have a pile-up on the concrete." << endl;
		cout << "\t" << numCrossingSago2MonkeyGrass << " crossing sago -> monkey grass" << endl;
		cout << "\t" << numCrossingMonkeyGrass2Sago << " crossing monkey grass -> sago" << endl;
		exit(-1);
  }

	// It takes a while to cross, so simulate it
	sleep(CROSS_SECONDS);

  // That one seems to have made it
  { // NN DS
    lock_guard<mutex> lock(crossing_mutex);
    numCrossingSago2MonkeyGrass--;
  }
}

/**
 * Tells others they can go now
 */
void Lizard::madeIt2MonkeyGrass() {
	// Whew, made it across, release spot
  sem_post(&driveway_sem); // NN DS

	if(debug) {
    lock_guard<mutex> lock(cout_mutex); // NN DS
		cout << "[" << _id << "] made the sago -> monkey grass crossing" << endl;
		cout << flush;
  }
}

/**
 * Simulate a lizard eating for a random amount of time
 */
void Lizard::eat() {
	int eatSeconds;

	eatSeconds = 1 + (int)(random() / (double)RAND_MAX * MAX_LIZARD_EAT);

	if(debug) {
    lock_guard<mutex> lock(cout_mutex); // NN DS
		cout << "[" << _id << "] eating for " << eatSeconds << " seconds" << endl;
		cout << flush;
  }

	// Simulate eating by blocking for a few seconds
	sleep(eatSeconds);

	if(debug) {
    lock_guard<mutex> lock(cout_mutex); // NN DS
    cout << "[" << _id << "] finished eating" << endl;
    cout << flush;
  }
}

/**
 * Returns when it is safe for this lizard to cross from the monkey
 * grass to the sago.
 */
void Lizard::monkeyGrass2SagoIsSafe() {
	if(debug) {
    lock_guard<mutex> lock(cout_mutex); // NN DS
		cout << "[" << _id << "] checking monkey grass -> sago" << endl;
		cout << flush;
  }

  // Wait for a spot on the driveway
  sem_wait(&driveway_sem); // NN DS

	if(debug) {
    lock_guard<mutex> lock(cout_mutex); // NN DS
		cout << "[" << _id << "] thinks monkey grass -> sago is safe" << endl;
		cout << flush;
  }
}

/**
 * Delays for 1 second to simulate crossing from the monkey
 * grass to the sago. 
 */
void Lizard::crossMonkeyGrass2Sago() {
	if(debug) {
    lock_guard<mutex> lock(cout_mutex); // NN DS
		cout << "[" << _id << "] crossing monkey grass -> sago" << endl;
		cout << flush;
  }

  // One more crossing this way
  { // NN DS
    lock_guard<mutex> lock(crossing_mutex);
	  numCrossingMonkeyGrass2Sago++;
  }

  // NN DS
  if(debug) {
    lock_guard<mutex> lock(cout_mutex);
		cout << numCrossingMonkeyGrass2Sago << " crossing monkey grass -> sago" << endl;
    cout << flush;
  }
  
  // Check for lizards cross both ways
	if(numCrossingSago2MonkeyGrass && UNIDIRECTIONAL) {
		cout << "\tOh No!, the lizards have cats all over them." << endl;
		cout << "\t " << numCrossingSago2MonkeyGrass << " crossing sago -> monkey grass" << endl;
		cout << "\t " << numCrossingMonkeyGrass2Sago << " crossing monkey grass -> sago" << endl;
		exit(-1);
  }

	// It takes a while to cross, so simulate it
	sleep(CROSS_SECONDS);

	// That one seems to have made it
  { // NN DS
    lock_guard<mutex> lock(crossing_mutex);
	  numCrossingMonkeyGrass2Sago--;
  }
}

/**
 * Tells others they can go now
 */
void Lizard::madeIt2Sago() {
  // Release a spot on the driveway
  sem_post(&driveway_sem); // NN DS

	// Whew, made it across
	if(debug) {
    lock_guard<mutex> lock(cout_mutex); // NN DS
		cout << "[" << _id << "] made the monkey grass -> sago crossing" << endl;
		cout << flush;
  }
}

 /**
  * Follows the algorithm provided in the assignment
  * description to simulate lizards crossing back and forth
  * between a sago palm and some monkey grass. 
  *  
  * @param aLizard - the lizard to be executed concurrently
  */
void Lizard::lizardThread(Lizard *aLizard) {
	if(debug) {
    lock_guard<mutex> lock(cout_mutex); // NN DS
    cout << "[" << aLizard->getId() << "] lizard is alive" << endl;
    cout << flush;
  }

	while(running) {
    // NN DS
    aLizard->sleepNow();
    aLizard->sago2MonkeyGrassIsSafe();
    aLizard->crossSago2MonkeyGrass();
    aLizard->madeIt2MonkeyGrass();
    aLizard->eat();
    aLizard->monkeyGrass2SagoIsSafe();
    aLizard->crossMonkeyGrass2Sago();
    aLizard->madeIt2Sago();
  }
}

/**
 * main()
 *
 * Should initialize variables, locks, semaphores, etc.
 * Should start the cat thread and the lizard threads.
 * Should block until all threads have terminated.
 */
int main(int argc, char **argv) {
	// Declare local variables
  vector<Lizard*> allLizards;
  vector<Cat*>    allCats; // NN DS

	// Check for the debugging flag (-d)
	debug = 0;
	if(argc > 1) {
		if(strncmp(argv[1], "-d", 2) == 0) {
			debug = 1;
    }
  }

	// Initialize variables
	numCrossingSago2MonkeyGrass = 0;
	numCrossingMonkeyGrass2Sago = 0;
	running = 1;

	// Initialize random number generator
	srandom((unsigned int)time(NULL));

	// Initialize locks and/or semaphores
  sem_init(&driveway_sem, 0, MAX_LIZARD_CROSSING); // NN DS

	// Create NUM_LIZARDS lizard threads
  for(int i = 0; i < NUM_LIZARDS; i++) {
    allLizards.push_back(new Lizard(i));
  }

  // Create NUM_CATS cat threads
  // NN DS
	for(int i = 0; i < NUM_CATS; i++) {
    allCats.push_back(new Cat(i));
  }

	// Run NUM_LIZARDS threads
  for(int i = 0; i < NUM_LIZARDS; i++) {
    allLizards[i]->run();
  }

  // Run NUM_CATS threads
  // NN DS
  for(int i = 0; i < NUM_CATS; i++) {
    allCats[i]->run();
  }

	// Now let the world run for a while
	sleep(WORLDEND);

  // That's it - the end of the world
	running = 0;

  // Wait until all lizard threads terminate
  // NN DS
  for(int i = 0; i < NUM_LIZARDS; i++) {
    allLizards[i]->wait();
  }

  // Wait until all cat threads terminate
  // NN DS
  for(int i = 0; i < NUM_CATS; i++) {
    allCats[i]->wait();
  }

	// Delete all lizard objects
  // NN DS
  for(int i = 0; i < NUM_LIZARDS; i++) {
    if(allLizards[i] != nullptr) {
      delete allLizards[i];
    }
  }

  // Delete all cat objects
  // NN DS
  for(int i = 0; i < NUM_CATS; i++) {
    if(allCats[i] != nullptr) {
      delete allCats[i];
    }
  }

  // Destroy the semaphore
  sem_destroy(&driveway_sem); // NN DS

  // Announce the end of the world
  if(debug) { // NN DS
    cout << "world ended" << endl;
    cout << flush;
  }
 
	// Exit happily
  return 0;
}