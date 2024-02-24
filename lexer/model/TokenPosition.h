// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 24/02/2024.
//

#pragma once

class TokenPosition {
public:

    /**
     * the line number of the token (zero-based)
     */
    unsigned int lineNumber;

    /**
     * the character number of the token relative to zero on the current line (zero-based)
     */
     unsigned int lineCharNumber;

     /**
      * position number relative to zero from the start of the file
      */
      unsigned int position;

      TokenPosition(unsigned int lineNumber, unsigned int lineCharNumber, unsigned int position) : lineNumber(lineNumber), lineCharNumber(lineCharNumber), position(position) {

      }

};