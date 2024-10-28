/**
 * File: lizardsUni.cpp
 * Authors: Noah Nickles, Dylan Stephens
 * Based on: lizards.cpp
 * Original Author: Dr. Reichherzer
 * Class: COP 4634 Systems & Networks I
 * 
 * Description:
 * This version of the Hungry Lizard Crossing project
 * implements unidirectional crossing. If multiple lizards are
 * crossing in opposite directions, the lizards run into each other
 * causing the cats to "play" with them.
 * Using a condition variable, a mutex, and a semaphore prevents
 * this from happening.
 */

// C Includes
#include <stdio.h>     // For standard I/O functions
#include <stdlib.h>    // For general utilities
#include <string.h>    // For string manipulation functions
#include <unistd.h>    // For sleep function
#include <semaphore.h> // For POSIX semaphores

// C++ Inlcudes
#include <condition_variable> // For thread synchronization
#include <iostream>           // For standard I/O stream
#include <mutex>              // For manging critial sections
#include <thread>             // For creating threads
#include <vector>             // For storing objects to create threads from

// Usings
using namespace std; // Cleans up code syntax a bit

// Constants
#define UNIDIRECTIONAL        1 // Restrict direction of lizards
#define WORLDEND             30 // Time in seconds for the simulation
#define NUM_LIZARDS          20 // Number of lizard threads to create
#define NUM_CATS              2 // Number of cat threads to create
#define MAX_LIZARD_CROSSING   4 // Max allowed lizards on the driveway simultaneously
#define MAX_LIZARD_SLEEP      3 // Max sleep time for lizards in seconds
#define MAX_CAT_SLEEP         3 // Max sleep time for cats in seconds
#define MAX_LIZARD_EAT        5 // Max time lizards spend eating in seconds
#define CROSS_SECONDS         2 // Time taken by a lizard to cross the driveway

// Classes/Enums
enum Direction {
  NONE,                 // No lizards currently crossing
  SAGO_TO_MONKEY_GRASS, // Lizards crossing from sago to monkey grass
  MONKEY_GRASS_TO_SAGO  // Lizards crossing from monkey grass to sago
};

/**
 * This class models a cat that sleep, wakes-up, checks on lizards in the driveway
 * and goes back to sleep. If the cat sees enough lizards it "plays" with them.
 */
class Cat {
	int     _id;   // Unique ID for each cat
	thread* _aCat; // Pointer to the cat's thread
	
	public:
		Cat(int id); // Constructor that initializes the cat's ID
		int getId(); // Getter for the cat's ID
		void run();  // Starts the cat's thread
		void wait(); // Waits for the cat's thread to complete
    
  private:
		void sleepNow();                   // Simulates the cat sleeping for a random time
    static void catThread (Cat *aCat); // Thread function for the cat
};

/**
 * This class simulates a lizard that alternates between sleeping, crossing the driveway,
 * eating, and returning back to the initial point to sleep.
 */
class Lizard {
	int     _id;      // Unique ID for each lizard
	thread* _aLizard; // Pointer to the lizard's thread
	
  public:
		Lizard(int id); // Constructor that initializes the lizard's ID
		int getId();    // Getter for the lizard's ID
    void run();     // Starts the lizard's thread
    void wait();    // Waits for the lizard's thread to complete

  private:
		void sago2MonkeyGrassIsSafe(); // Checks if it is safe to cross from sago to monkey grass
		void crossSago2MonkeyGrass();  // Crosses the driveway from sago to monkey grass
		void madeIt2MonkeyGrass();     // Completes crossing to monkey grass and releases a driveway spot
		void eat();                    // Simulates the lizard eating
		void monkeyGrass2SagoIsSafe(); // Checks if it is safe to cross from monkey grass to sago
		void crossMonkeyGrass2Sago();  // Crosses the driveway from monkey grass to sago
		void madeIt2Sago();            // Completes crossing to sago and releases a driveway spot
		void sleepNow();               // Simulates the lizard sleeping
    static void lizardThread(Lizard *aLizard); // Thread function for the lizard
};

// Synchronization Globals
Direction currentDirection = NONE; // Tracks the current crossing direction of lizards
condition_variable direction_CV;   // Condition variable for direction control
mutex direction_mutex;             // Mutex for direction control
mutex cout_mutex;                  // Mutex to control access to standard output
sem_t driveway_sem;                // Semaphore to limit the number of lizards on the driveway

// Global Variables
int numCrossingSago2MonkeyGrass = 0; // Count of lizards crossing from sago to monkey grass
int numCrossingMonkeyGrass2Sago = 0; // Count of lizards crossing from monkey grass to sago
int debug   = 0;                     // Debug mode flag
int running = 1;                     // Flag to keep the simulation running

// Cat Class Methods

/**
 * Constructs a cat with a unique ID.
 *
 * @param id - Unique ID for the cat.
 */
Cat::Cat (int id) {
	_id = id;
}

/**
 * Returns the ID of the cat.
 *
 * @return Unique ID for the cat.
 */
int Cat::getId() {
	return _id;
}

/**
 * Launches a cat thread if it hasn't been started.
 */
void Cat::run() {
  if(!_aCat) {
    _aCat = new thread(catThread, this);
  }
}

/**
 * Waits for the cat thread to complete execution.
 */
void Cat::wait() {
  if(_aCat && _aCat->joinable()) {
    _aCat->join();
  }
}

/**
 * Simulates the cat sleeping for a random amount of time.
 */
void Cat::sleepNow() {
	int sleepSeconds = 1 + (int)(random() / (double)RAND_MAX * MAX_CAT_SLEEP);

	if(debug) {
    lock_guard<mutex> lock(cout_mutex);
		cout << "[" << _id << "] cat sleeping for " << sleepSeconds << " seconds" << endl;
		cout << flush;
  }

	sleep(sleepSeconds);

	if(debug) {
    lock_guard<mutex> lock(cout_mutex);
		cout << "[" << _id << "] cat awake" << endl;
		cout << flush;
  }
}

/**
 * Cat's main thread function that repeatedly sleeps and checks for
 * lizard traffic on the driveway.
 * 
 * @param aCat - Pointer to the cat instance.
 */
void Cat::catThread(Cat *aCat) {
	if(debug) {
    lock_guard<mutex> lock(cout_mutex);
		cout << "[" << aCat->getId() << "] cat is alive\n";
		cout << flush;
  }

	while(running) {
		aCat->sleepNow();

		// Check if too many lizards are on the driveway
    int totalCrossing = numCrossingSago2MonkeyGrass + numCrossingMonkeyGrass2Sago; // NN DS
		if(totalCrossing > MAX_LIZARD_CROSSING) {
      lock_guard<mutex> lock(cout_mutex);
		  cout << "\tThe cats are happy - they have toys.\n";
      exit(-1);
		}
  }
}

// Lizard Class Methods

/**
 * Constructs a lizard with a unique ID.
 *
 * @param id - Unique ID for the lizard.
 */
Lizard::Lizard(int id) {
	_id = id;
}

/**
 * Returns the ID of the lizard.
 *
 * @return Unique ID for the lizard.
 */
int Lizard::getId() {
	return _id;
}

/**
 * Launches a lizard thread if it hasn't been started.
 */
void Lizard::run() {
  if(!_aLizard) {
    _aLizard = new thread(lizardThread, this);
  }
}
 
/**
 * Waits for the lizard thread to complete execution.
 */
void Lizard::wait() {
	if(_aLizard && _aLizard->joinable()) {
    _aLizard->join();
  } 
}

/**
 * Simulates a lizard sleeping for a random amount of time.
 */
void Lizard::sleepNow() {
	int sleepSeconds = 1 + (int)(random() / (double)RAND_MAX * MAX_LIZARD_SLEEP);

	if(debug) {
    lock_guard<mutex> lock(cout_mutex);
    cout << "[" << _id << "] sleeping for " << sleepSeconds << " seconds" << endl;
    cout << flush;
  }

	sleep(sleepSeconds);

	if(debug) {
    lock_guard<mutex> lock(cout_mutex);
    cout << "[" << _id << "] awake" << endl;
    cout << flush;
  }
}

/**
 * Checks if it is safe for the lizard to start crossing from the sago
 * to the monkey grass.
 */
void Lizard::sago2MonkeyGrassIsSafe() {
	if(debug) {
    lock_guard<mutex> lock(cout_mutex);
		cout << "[" << _id << "] checking sago -> monkey grass" << endl;
		cout << flush;
  }

  // Wait for a spot on the driveway if at max capacity
  sem_wait(&driveway_sem);

  // Lock the direction for crossing
  unique_lock<mutex> lock(direction_mutex);

  // Wait until no lizards are crossing in the opposite direction
  direction_CV.wait(lock, [] {
    return (currentDirection == SAGO_TO_MONKEY_GRASS && 
            numCrossingMonkeyGrass2Sago == 0) || 
            currentDirection == NONE;
  });

  // Set the direction for crossing if it is not already set
  if(currentDirection == NONE) {
    currentDirection = SAGO_TO_MONKEY_GRASS;
  }

  // Claim intent to start crossing
  numCrossingSago2MonkeyGrass++;

	if(debug) {
    lock_guard<mutex> lock(cout_mutex);
		cout << "[" << _id << "] thinks sago -> monkey grass is safe" << endl;
		cout << flush;
  }
}

/**
 * Simulates the lizard actively crossing the driveway 
 * from the sago to the monkey grass.
 */
void Lizard::crossSago2MonkeyGrass() {
	if(debug) {
    lock_guard<mutex> lock(cout_mutex);
    cout << "[" << _id << "] crossing  sago -> monkey grass" << endl;
    cout << flush;
  }

  {
    lock_guard<mutex> lock(direction_mutex);

    // Check for crossing conflicts
    if(numCrossingMonkeyGrass2Sago > 0 && UNIDIRECTIONAL) {
      cout << "\tCrash!  We have a pile-up on the concrete." << endl;
      cout << "\t" << numCrossingSago2MonkeyGrass << " crossing sago -> monkey grass" << endl;
      cout << "\t" << numCrossingMonkeyGrass2Sago << " crossing monkey grass -> sago" << endl;
      exit(-1);
    }
  }

  // NN DS
  if(debug) {
    lock_guard<mutex> lock(cout_mutex);
		cout << numCrossingSago2MonkeyGrass << " crossing sago -> monkey grass" << endl;
    cout << flush;
  }

	// Simulate the time taken to cross the driveway
	sleep(CROSS_SECONDS);

  // Mark crossing completion and update counters
  {
    lock_guard<mutex> lock(direction_mutex);
    numCrossingSago2MonkeyGrass--;

    // If no lizards are left in this direction, release the direction lock
    if(numCrossingSago2MonkeyGrass == 0) {
      currentDirection = NONE;
      direction_CV.notify_all();
    }
  }
}

/**
 * Signals that the lizard has safely crossed to the monkey grass side.
 * Releases one spot on the driveway semaphore.
 */
void Lizard::madeIt2MonkeyGrass() {
	// Release driveway spot
  sem_post(&driveway_sem);

	if(debug) {
    lock_guard<mutex> lock(cout_mutex);
		cout << "[" << _id << "] made the sago -> monkey grass crossing" << endl;
		cout << flush;
  }
}

/**
 * Simulates the lizard eating for a random amount of time after crossing.
 */
void Lizard::eat() {
	int eatSeconds = 1 + (int)(random() / (double)RAND_MAX * MAX_LIZARD_EAT);

	if(debug) {
    lock_guard<mutex> lock(cout_mutex);
		cout << "[" << _id << "] eating for " << eatSeconds << " seconds" << endl;
		cout << flush;
  }

	sleep(eatSeconds);

	if(debug) {
    lock_guard<mutex> lock(cout_mutex);
    cout << "[" << _id << "] finished eating" << endl;
    cout << flush;
  }
}

/**
 * Checks if it is safe for the lizard to cross from the monkey grass
 * back to the sago.
 */
void Lizard::monkeyGrass2SagoIsSafe() {
	if(debug) {
    lock_guard<mutex> lock(cout_mutex);
		cout << "[" << _id << "] checking monkey grass -> sago" << endl;
		cout << flush;
  }

  // Wait for a spot on the driveway if at max capacity
  sem_wait(&driveway_sem);

  // Lock the direction for crossing
  unique_lock<mutex> lock(direction_mutex);

  // Wait until no lizards are crossing in the opposite direction
  direction_CV.wait(lock, [] {
    return (currentDirection == MONKEY_GRASS_TO_SAGO &&
            numCrossingSago2MonkeyGrass == 0) ||
            currentDirection == NONE;
  });

  // Set the direction for crossing if it is not already set
  if(currentDirection == NONE) {
    currentDirection = MONKEY_GRASS_TO_SAGO;
  }

  // Claim intent to start crossing
  numCrossingMonkeyGrass2Sago++;

	if(debug) {
    lock_guard<mutex> lock(cout_mutex);
		cout << "[" << _id << "] thinks monkey grass -> sago is safe" << endl;
		cout << flush;
  }
}

/**
 * Simulates the lizard crossing the driveway back to the sago.
 */
void Lizard::crossMonkeyGrass2Sago() {
	if(debug) {
    lock_guard<mutex> lock(cout_mutex);
		cout << "[" << _id << "] crossing monkey grass -> sago" << endl;
		cout << flush;
  }

  {
    lock_guard<mutex> lock(direction_mutex);

    // Check for crossing conflicts
    if(numCrossingSago2MonkeyGrass > 0 && UNIDIRECTIONAL) {
      cout << "\tOh No!, the lizards have cats all over them." << endl;
      cout << "\t " << numCrossingSago2MonkeyGrass << " crossing sago -> monkey grass" << endl;
      cout << "\t " << numCrossingMonkeyGrass2Sago << " crossing monkey grass -> sago" << endl;
      exit(-1);
    }
  }

  if(debug) {
    lock_guard<mutex> lock(cout_mutex);
		cout << numCrossingMonkeyGrass2Sago << " crossing monkey grass -> sago" << endl;
    cout << flush;
  }

	sleep(CROSS_SECONDS);

	// Mark crossing completion and update counters
  {
    lock_guard<mutex> lock(direction_mutex);
    numCrossingMonkeyGrass2Sago--;

    // If no lizards are left in this direction, release the direction lock
    if(numCrossingMonkeyGrass2Sago == 0) {
      currentDirection = NONE;
      direction_CV.notify_all();
    }
  }
}

/**
 * Signals tha the lizard has safely crossed back to the sago side,
 * releasing a spot on the driveway semaphore.
 */
void Lizard::madeIt2Sago() {
  // Release driveway spot
  sem_post(&driveway_sem);

	if(debug) {
    lock_guard<mutex> lock(cout_mutex);
		cout << "[" << _id << "] made the monkey grass -> sago crossing" << endl;
		cout << flush;
  }
}

 /**
  * Follows the algorithm provided in the assignment
  * description to simulate lizards crossing back and forth
  * between a sago palm and some monkey grass. 
  *  
  * @param aLizard - Pointer to the lizard thread.
  */
void Lizard::lizardThread(Lizard *aLizard) {
	if(debug) {
    lock_guard<mutex> lock(cout_mutex);
    cout << "[" << aLizard->getId() << "] lizard is alive" << endl;
    cout << flush;
  }

	while(running) {
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

// Main

/**
 * Initializes and runs the simulation, creates all lizard and cat threads,
 * and manages cleanup.
 */
int main(int argc, char **argv) {
	// Check for the debugging flag (-d)
	if(argc > 1) {
		if(strncmp(argv[1], "-d", 2) == 0) {
			debug = 1;
    }
  }

	// Declare thread vectors
  vector<Lizard*> allLizards;
  vector<Cat*>    allCats;

	// Initialize random number generator
	srandom((unsigned int)time(NULL));

	// Initialize semaphore to control max number of lizards on the driveway
  sem_init(&driveway_sem, 0, MAX_LIZARD_CROSSING);

	// Create all lizard and cat threads and store in vectors
  for(int i = 0; i < NUM_LIZARDS; i++) {
    allLizards.push_back(new Lizard(i));
  }
	for(int i = 0; i < NUM_CATS; i++) {
    allCats.push_back(new Cat(i));
  }

	// Run all lizard and cat threads
  for(auto& lizard : allLizards) {
    lizard->run();
  }
  for(auto& cat : allCats) {
    cat->run();
  }

	// Now let the world run for a while
	sleep(WORLDEND);

  // That's it - the end of the world
	running = 0;

  // Wait until all lizard and cat threads terminate
  for(auto& lizard : allLizards) {
    lizard->wait();
  }
  for(auto& cat : allCats) {
    cat->wait();
  }

  // Delete all lizard and cat objects
  for(auto& lizard : allLizards) {
    delete lizard;
  }
  for(auto& cat : allCats) {
    delete cat;
  }

  // Destroy semaphore
  sem_destroy(&driveway_sem);

  // Announce the end of the world
  if(debug) {
    cout << "world ended" << endl;
    cout << flush;
  }
 
	// Exit happily
  return 0;
}