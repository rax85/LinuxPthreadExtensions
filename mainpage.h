/**
 * @file mainpage.h
 * @mainpage Linux PThread Extensions.
 *  
 * 
 * This library provides some functionality that I thought would be a nice
 * addition to the stock pthread library. 
 * 
 * I. This is a list of currently available and planned functionality:<br>
 *     1. Thread Pools.
 *         - Fixed sized thread pools are minimally tested.
 *         - Variable sized thread pools are minimally tested.
 *         - Group execution of threads. Same callback, multiple threads [Planned].
 *         - Barriers are implemented and minimally tested.
 *     
 *     2. Semaphores.<br>
 *         - The blocking semaphore is implemented and minimally tested. This is
 *           built entirely using pthread routines and not posix semaphores.
 *         - Timed semaphores are implemented and minimally tested.
 *     
 *     3. Memory pools<br>
 *         - Memory pools with support for fixed sized allocation are implemented and 
 *           minimally tested.
 *         - Memory pools with support for variable sized allocation is now coded and 
 *           minimally tested.
 * 
 * II. Building.<br>
 *     - 'make' builds the library.
 *     - 'make documentation' builds doxygen documentation. You need doxygen for this
 *       target to build.
 *     - 'make test' builds test.out a binary that runs a few simple tests.
 * 
 * III. Linking notes.<br>
 *     - While linking against your program, be sure to also link 
 *       against -lpthread and -lrt
 * 
 * Keep checking back for developments!
 * <br>
 * <br>
 *
 * LICENSE:<br> 
 * <br>
 * This program is free software: you can redistribute it and/or modify<br>
 * it under the terms of the GNU Lesser General Public License as published by<br>
 * the Free Software Foundation, either version 3 of the License, or<br>
 * (at your option) any later version.<br>
 * <br>
 * This program is distributed in the hope that it will be useful,<br>
 * but WITHOUT ANY WARRANTY; without even the implied warranty of<br>
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the<br>
 * GNU Lesser General Public License for more details.<br>
 * <br>
 * You should have received a copy of the GNU Lesser General Public License<br>
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.<br>
 */
