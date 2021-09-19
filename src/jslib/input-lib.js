mergeInto(LibraryManager.library, {
  findAutoInputConfigName: function(gamepadNamePtr, responseBufferPointer, maxCharacters) {
    
    Asyncify.handleSleep((wakeUp) => {

      const gamepadName = UTF8ToString(gamepadNamePtr);
      
      Module.findAutoInputConfig(gamepadName).then((result) => {
        console.log("found autoInputConfig: %o", result);

        let response = "";
        
        if (result.matchScore > 0) {
          response = result.matchName;
        }

        stringToUTF8(response, responseBufferPointer, maxCharacters);
        
        wakeUp();
      });
    });    
  }
});
