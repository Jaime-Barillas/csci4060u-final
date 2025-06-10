/* NOTE: Very hacky and potentially dangerous!
 * Abuse Pony's return-type polymorphism to "cast" an SDL_Event to a specific
 * event type. */
void * PSDL_ConvertEvent(void * event) { return event; }
