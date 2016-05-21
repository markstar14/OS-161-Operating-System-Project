/*
 * Copyright (c) 2001, 2002, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Driver code for whale mating problem
 */
#include <types.h>
#include <lib.h>
#include <thread.h>
#include <test.h>
#include <synch.h>

/*
 * 08 Feb 2012 : GWA : Driver code is in kern/synchprobs/driver.c. We will
 * replace that file. This file is yours to modify as you see fit.
 *
 * You should implement your solution to the whalemating problem below.
 */

// 13 Feb 2012 : GWA : Adding at the suggestion of Isaac Elbaz. These
// functions will allow you to do local initialization. They are called at
// the top of the corresponding driver code.
static struct cv *male_cv;

static struct cv *female_cv;

static struct cv *match_cv;

static struct lock *male_lock;

static struct lock *female_lock;

static struct lock *match_lock;

int malecount = 0;

int femalecount = 0;

int matchcount = 0;

void whalemating_init() {
	male_cv = cv_create("male_cv");
	if(male_cv == NULL)
                panic("Couldn't create male_cv");

	female_cv = cv_create("female_cv");
	if(female_cv == NULL)
                panic("Couldn't create female_cv");
        
	match_cv = cv_create("match_cv");
	if(match_cv == NULL)
		panic("Couldn't create match_cv");
        
	male_lock = lock_create("male_lock");
        if(male_lock == NULL)
                panic("Couldn't create male_lock");
        
	female_lock = lock_create("female_lock");
        if(female_lock == NULL)
                panic("Couldn't create female_lock");
      	
	match_lock = lock_create("match_lock");
        if(match_lock == NULL)
                panic("Couldn't create match_lock");

	return;
}

// 20 Feb 2012 : GWA : Adding at the suggestion of Nikhil Londhe. We don't
// care if your problems leak memory, but if you do, use this to clean up.

void whalemating_cleanup() {
  return;
}

void
male(void *p, unsigned long which)
{
	struct semaphore * whalematingMenuSemaphore = (struct semaphore *)p;
  	(void)which;
//	male_start();
	lock_acquire(male_lock);
	male_start();
//	kprintf("male %ld starting\n", which);
	malecount++;
	cv_signal(match_cv, match_lock);
//	cv_broadcast(female_cv, male_lock);
//	cv_broadcast(match_cv, male_lock);
//	malecount++;
//	while(!femalecount || !matchcount){
		cv_wait(male_cv, male_lock);
//		cv_signal(female_cv, male_lock);
//        	cv_signal(match_cv, male_lock);
//		;
//	}
//	cv_signal(female_cv, female_lock);
//	cv_signal(match_cv, match_lock);
//	kprintf("male %ld mating\n", which);
//	malecount--;
//	cv_broadcast(male_cv, male_lock);
	male_end();
//	kprintf("male %ld ending\n", which);
	lock_release(male_lock);
//	cv_signal(male_cv, male_lock);
//	male_end();
  // 08 Feb 2012 : GWA : Please do not change this code. This is so that your
  // whalemating driver can return to the menu cleanly.
 	V(whalematingMenuSemaphore);
  return;
}

void
female(void *p, unsigned long which)
{
	struct semaphore * whalematingMenuSemaphore = (struct semaphore *)p;
	(void)which;
//	female_start();
  	lock_acquire(female_lock);
	female_start();
//	kprintf("female %ld starting\n", which);
        femalecount++;
	cv_signal(match_cv, match_lock);
//	cv_broadcast(male_cv, female_lock);
//        cv_broadcast(match_cv, female_lock);
//	femalecount++; 
//       while(!malecount || !matchcount){
              cv_wait(female_cv, female_lock);
//		cv_signal(male_cv, female_lock);
//        	cv_signal(match_cv, female_lock);
//		;
//	}
//	cv_signal(male_cv, male_lock);
//	cv_signal(match_cv, match_lock);
//  	kprintf("female %ld mating\n", which);
//	femalecount--;
	female_end();
//	kprintf("female %ld ending\n", which);
	lock_release(female_lock);
//	cv_signal(female_cv, female_lock);
//	female_end();
  // 08 Feb 2012 : GWA : Please do not change this code. This is so that your
  // whalemating driver can return to the menu cleanly.
  V(whalematingMenuSemaphore);
  return;
}

void
matchmaker(void *p, unsigned long which)
{
	struct semaphore * whalematingMenuSemaphore = (struct semaphore *)p;
 	(void)which;
//	matchmaker_start();
  	lock_acquire(match_lock);
	matchmaker_start();
//	kprintf("%ld match making starting\n", which);
	matchcount++;
//	cv_broadcast(male_cv, match_lock);
//	cv_broadcast(female_cv, match_lock);
//	matchcount++;
	while(!malecount || !femalecount){
		cv_wait(match_cv, match_lock);
//		cv_signal(male_cv, match_lock);
//        	cv_signal(female_cv, match_lock);
//		;
	}	
	cv_signal(male_cv, male_lock);
	cv_signal(female_cv, female_lock);
//	kprintf("%ld match making\n", which);
	matchcount--;
	malecount--;
	femalecount--;
	matchmaker_end();
//	kprintf("%ld match making ending\n", which);
	lock_release(match_lock);
//	cv_signal(match_cv, match_lock);
//	matchmaker_end();
  // 08 Feb 2012 : GWA : Please do not change this code. This is so that your
  // whalemating driver can return to the menu cleanly.
  V(whalematingMenuSemaphore);
  return;
}

/*
 * You should implement your solution to the stoplight problem below. The
 * quadrant and direction mappings for reference: (although the problem is,
 * of course, stable under rotation)
 *
 *   | 0 |
 * --     --
 *    0 1
 * 3       1
 *    3 2
 * --     --
 *   | 2 | 
 *
 * As way to think about it, assuming cars drive on the right: a car entering
 * the intersection from direction X will enter intersection quadrant X
 * first.
 *
 * You will probably want to write some helper functions to assist
 * with the mappings. Modular arithmetic can help, e.g. a car passing
 * straight through the intersection entering from direction X will leave to
 * direction (X + 2) % 4 and pass through quadrants X and (X + 3) % 4.
 * Boo-yah.
 *
 * Your solutions below should call the inQuadrant() and leaveIntersection()
 * functions in drivers.c.
 */

// 13 Feb 2012 : GWA : Adding at the suggestion of Isaac Elbaz. These
// functions will allow you to do local initialization. They are called at
// the top of the corresponding driver code.
static struct lock *lock_0;
static struct lock *lock_1;
static struct lock *lock_2;
static struct lock *lock_3;

static struct semaphore *sem_intersect;

void stoplight_init() 
{
	lock_0 = lock_create("quadr 0");
	if(lock_0 == NULL)
		panic("stoplight: lock_create failed");
	lock_1 = lock_create("quadr 1");       
	if(lock_1 == NULL)
                panic("stoplight: lock_create failed");
	lock_2 = lock_create("quadr 2");
        if(lock_2 == NULL)
                panic("stoplight: lock_create failed");
	lock_3 = lock_create("quadr 3");
        if(lock_3 == NULL)
                panic("stoplight: lock_create failed");

	sem_intersect = sem_create("Intersection semaphore", 3);
	
	return;
}

// 20 Feb 2012 : GWA : Adding at the suggestion of Nikhil Londhe. We don't
// care if your problems leak memory, but if you do, use this to clean up.

void stoplight_cleanup() {
  return;
}

void
gostraight(void *p, unsigned long direction)
{
	struct semaphore * stoplightMenuSemaphore = (struct semaphore *)p;
	(void)direction;

	P(sem_intersect);

	switch(direction)
	{
		case 0:
		{
			lock_acquire(lock_0);
			inQuadrant(0);
			lock_acquire(lock_3);
			inQuadrant(3);
			lock_release(lock_0);
			leaveIntersection();
			lock_release(lock_3);
			break;
		}
		
		case 1: 
                {
                        lock_acquire(lock_1);
                        inQuadrant(1);
                        lock_acquire(lock_0);
			inQuadrant(0);
                        lock_release(lock_1);
                        leaveIntersection();
                        lock_release(lock_0);
                        break;
                }
		
		case 2: 
                {
                        lock_acquire(lock_2);
                        inQuadrant(2);
                        lock_acquire(lock_1);
			inQuadrant(1);
                        lock_release(lock_2);
                        leaveIntersection();
                        lock_release(lock_1);
                        break;
                }
		
		case 3: 
                {
                        lock_acquire(lock_3);
                        inQuadrant(3);
                        lock_acquire(lock_2);
			inQuadrant(2);
                        lock_release(lock_3);
                        leaveIntersection();
                        lock_release(lock_2);
                        break;
                }
	}
	
	V(sem_intersect);
	
	// 08 Feb 2012 : GWA : Please do not change this code. This is so that 
  	// your stoplight driver can return to the menu cleanly.
  	V(stoplightMenuSemaphore);
  	return;
}

void
turnleft(void *p, unsigned long direction)
{
	struct semaphore * stoplightMenuSemaphore = (struct semaphore *)p;
  	(void)direction;
  	
	P(sem_intersect);

	switch(direction)
	{
		case 0:
		{
			lock_acquire(lock_0);
			inQuadrant(0);
			lock_acquire(lock_3);
			inQuadrant(3);
			lock_release(lock_0);
			lock_acquire(lock_2);
			inQuadrant(2);
			lock_release(lock_3);
			leaveIntersection();
			lock_release(lock_2);
			break;
		}
		
		case 1:
                {
                        lock_acquire(lock_1);
                        inQuadrant(1);
                        lock_acquire(lock_0);
			inQuadrant(0);
                        lock_release(lock_1);
                        lock_acquire(lock_3);
			inQuadrant(3);
                        lock_release(lock_0);
                        leaveIntersection();
                        lock_release(lock_3);
                        break;
                }
  		
		case 2:
                {
                        lock_acquire(lock_2);
                        inQuadrant(2);
                        lock_acquire(lock_1);
			inQuadrant(1);
                        lock_release(lock_2);
                        lock_acquire(lock_0);
			inQuadrant(0);
                        lock_release(lock_1);
                        leaveIntersection();
                        lock_release(lock_0);
                        break;
                }
  		case 3:
                {
                        lock_acquire(lock_3);
                        inQuadrant(3);
                        lock_acquire(lock_2);
			inQuadrant(2);
                        lock_release(lock_3);
                        lock_acquire(lock_1);
			inQuadrant(1);
                        lock_release(lock_2);
                        leaveIntersection();
                        lock_release(lock_1);
                        break;
                }
	}
	
	V(sem_intersect);

  	// 08 Feb 2012 : GWA : Please do not change this code. This is so that 
	// your stoplight driver can return to the menu cleanly.
  	V(stoplightMenuSemaphore);
  	return;
}

void
turnright(void *p, unsigned long direction)
{
	struct semaphore * stoplightMenuSemaphore = (struct semaphore *)p;
  	(void)direction;
	
	P(sem_intersect);
	
	switch(direction)
	{
		case 0:
		{
			lock_acquire(lock_0);
			inQuadrant(0);
			leaveIntersection();
			lock_release(lock_0);
			break;
		}

                case 1:
                {
                        lock_acquire(lock_1);
                        inQuadrant(1);
                        leaveIntersection();
                        lock_release(lock_1);
                        break;
                }
                case 2:
                {
                        lock_acquire(lock_2);
                        inQuadrant(2);
                        leaveIntersection();
                        lock_release(lock_2);
                        break;
                }
                
		case 3:
                {
                        lock_acquire(lock_3);
                        inQuadrant(3);
                        leaveIntersection();
                        lock_release(lock_3);
                        break;
                }
	}
	
	V(sem_intersect);
  	
	// 08 Feb 2012 : GWA : Please do not change this code. This is so that y	// your stoplight driver can return to the menu cleanly.
  	V(stoplightMenuSemaphore);
  	return;
}
