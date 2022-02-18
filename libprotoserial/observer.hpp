/*  
 * a way to provide one way communication between objects through functions
 * regardless of the arguments
 * 
 * subject.emit(data, adress, port)
 *  -> observer1
 *  ..
 *  -> observerN
 * 
 * subject can be a separate class from the the object that is actually creating 
 * the event since the data can be passed into the subjects easily
 * 
 * observers must be member functions of objects since they usually need to keep
 * their own data and contexts - this is where the problem arises since now we can't
 * teplate the entire object because observers may want to subscribe to multiple
 * subjects who each can have different signatures
 * 
 * subjects must be templates of the observer's function signature, or rather 
 * only such observers can subscribe who match the signature of the subject
 * 
 * 
 *  
 */



namespace sp
{
    class subject
    {

    };


}



