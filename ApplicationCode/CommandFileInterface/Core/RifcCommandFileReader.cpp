/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2017 Statoil ASA
// 
//  ResInsight is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
// 
//  ResInsight is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or
//  FITNESS FOR A PARTICULAR PURPOSE.
// 
//  See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html> 
//  for more details.
//
/////////////////////////////////////////////////////////////////////////////////

#include "RifcCommandFileReader.h"

#include "RicfCommandObject.h"
#include "RicfObjectCapability.h"

#include "cafPdmObjectFactory.h"

#include <QTextStream>

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
std::vector<RicfCommandObject*> RicfCommandFileReader::readCommands(QTextStream& inputStream, 
                                                                    caf::PdmObjectFactory* objectFactory)
{
    std::vector<RicfCommandObject*> readCommands;

    while ( !inputStream.atEnd() )
    {
        inputStream.skipWhiteSpace();

        // Read command name
        QString commandName;
        bool foundStartBracet = false;
        {
            inputStream.skipWhiteSpace();
            while ( !inputStream.atEnd() )
            {
                QChar currentChar;
                inputStream >> currentChar;
                if ( currentChar.isSpace() )
                {
                    inputStream.skipWhiteSpace();
                    QChar isBracket('a');
                    inputStream >> isBracket;
                    if ( isBracket != QChar('(') )
                    {
                        // Error, could not find start bracket for command
                        return readCommands;
                    }
                    break;
                }
                else if ( currentChar == QChar('(') )
                {
                    break;
                }
                commandName += currentChar;
            }
        }

        CAF_ASSERT(objectFactory);
        caf::PdmObjectHandle* obj = objectFactory->create(commandName);
        RicfCommandObject* cObj = dynamic_cast<RicfCommandObject*>(obj);

        if ( cObj == nullptr )
        {
            // Error: Unknown command
            // Skip to end of command
            QChar currentChar;
            bool isOutsideQuotes = true;
            while ( !inputStream.atEnd() )
            {
                inputStream >> currentChar;
                if ( isOutsideQuotes )
                {
                    if ( currentChar == QChar(')') )
                    {
                        break;
                    }
                    if ( currentChar == QChar('\"') )
                    {
                        isOutsideQuotes = false;
                    }
                }
                else
                {
                    if ( currentChar == QChar('\"') )
                    {
                        isOutsideQuotes = true;
                    }

                    if ( currentChar == QChar('\\') )
                    {
                        inputStream >> currentChar;
                    }
                }
            }
        }
        else
        {
            readCommands.push_back(cObj);
            auto rcfCap = cObj->capability<RicfObjectCapability>();
            rcfCap->readFields(inputStream, objectFactory);
        }
    }

    return readCommands;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RicfCommandFileReader::writeCommands(QTextStream& outputStream, const std::vector<RicfCommandObject*>& commandsToWrite)
{

}