/**
* (C) 2018 Volvo Cars
*
* All files and artifacts in this repository are licensed under the
* provisions of the license provided by the LICENSE file in this repository.
*
*
* Write native format to file.
**/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>
#include "vssparserutilities.h"  //nativeCnodeDef.h uses it. Maybe some methods could be used also?
#include "nativeCnodeDef.h"

FILE* treeFp;
int objectType = -1;  // declared at this level to propagate type from rbranch context to its element children contexts

int stringToTypeDef(char* type) {
    if (strcmp(type, "Int8") == 0 || strcmp(type, "int8") == 0)
        return INT8;
    if (strcmp(type, "UInt8") == 0 || strcmp(type, "uint8") == 0)
        return UINT8;
    if (strcmp(type, "Int16") == 0 || strcmp(type, "int16") == 0)
        return INT16;
    if (strcmp(type, "UInt16") == 0 || strcmp(type, "uint16") == 0)
        return UINT16;
    if (strcmp(type, "Int32") == 0 || strcmp(type, "int32") == 0)
        return INT32;
    if (strcmp(type, "UInt32") == 0 || strcmp(type, "uint32") == 0)
        return UINT32;
    if (strcmp(type, "Double") == 0 || strcmp(type, "double") == 0)
        return DOUBLE;
    if (strcmp(type, "Float") == 0 || strcmp(type, "float") == 0)
        return FLOAT;
    if (strcmp(type, "Boolean") == 0 || strcmp(type, "boolean") == 0)
        return BOOLEAN;
    if (strcmp(type, "String") == 0 || strcmp(type, "string") == 0)
        return STRING;
    if (strcmp(type, "sensor") == 0)
        return SENSOR;
    if (strcmp(type, "actuator") == 0)
        return ACTUATOR;
    if (strcmp(type, "stream") == 0)
        return STREAM;
    if (strcmp(type, "attribute") == 0)
        return ATTRIBUTE;
    if (strcmp(type, "branch") == 0)
        return BRANCH;
    if (strcmp(type, "rbranch") == 0)
        return RBRANCH;
    if (strcmp(type, "element") == 0)
        return ELEMENT;
    printf("Unknown type! |%s|\n", type);
    return -1;
}

int countEnumElements(char* enums) {
    int delimiters = 0;
    for (int i = 0 ; i < strlen(enums) ; i++) {
        if (enums[i] == '/')
            delimiters++;
    }
    if (delimiters == 0)
        return 0;
    return delimiters-1;
}

char* extractEnumElement(char* enums, int index, char* buf) {
    char* enumstart = enums;
    char* enumend  = NULL;
    for (int i = 0 ; i < index+1 ; i++) {
        enumend = strchr(enumstart+1, '/');
        if (enumend == NULL)
            return NULL;
        if (i < index)
            enumstart = enumend;
    }
    strncpy(buf, enumstart+1, (int)(enumend-(enumstart+1)));
    buf[(int)(enumend-(enumstart+1))] = '\0';
    return buf;
}

void writeCommonPart(char* name, char* type, char* uuid, char* descr, int children) {
    common_node_data_t commonData;

    commonData.nameLen  = strlen(name);
    commonData.type = stringToTypeDef(type);
    commonData.uuidLen  = strlen(uuid);
    commonData.descrLen = strlen(descr);
    commonData.children = children;

    fwrite(&commonData, sizeof(common_node_data_t), 1, treeFp);
    fwrite(name, sizeof(char)*commonData.nameLen, 1, treeFp);
    fwrite(uuid, sizeof(char)*commonData.uuidLen, 1, treeFp);
    fwrite(descr, sizeof(char)*commonData.descrLen, 1, treeFp);
}

void writeNodeData(char* name, char* type, char* uuid, char* descr, int children, char* datatype, char* min, char* max, char* unit, char* enums, char* function) {
printf("Name=%s, Type=%s, uuid=%s, children=%d, Descr=%s, datatype=%s, min=%s, max=%s Unit=%s, Enums=%s, function=%s\n", name, type, uuid, children, descr, datatype, min, max, unit, enums, function);
    writeCommonPart(name, type, uuid, descr, children);
    int dtype = -1;
    if (strlen(datatype) != 0)
        dtype = stringToTypeDef(datatype);
    fwrite(&dtype, sizeof(int), 1, treeFp);
    int nodeMin = INT_MAX;
    if (strlen(min) != 0)
        nodeMin = atoi(min);
    fwrite(&nodeMin, sizeof(int), 1, treeFp);
    int nodeMax = INT_MIN;
    if (strlen(max) != 0)
        nodeMax = atoi(max);
    fwrite(&nodeMax, sizeof(int), 1, treeFp);
    int unitLen = (int)strlen(unit);
    fwrite(&unitLen, sizeof(int), 1, treeFp);
    if (unitLen > 0)
        fwrite(unit, sizeof(char)*unitLen, 1, treeFp);
    int numOfEnumElements = countEnumElements(enums);
//printf("Num of enum elements=%d\n", numOfEnumElements);
    fwrite(&numOfEnumElements, sizeof(int), 1, treeFp);
    if (numOfEnumElements > 0) {
        enum_t* enumeration;
        enumeration = malloc(sizeof(enum_t)*numOfEnumElements);
        if (enumeration == NULL) {
            printf("traverseAndSaveNode:malloc failed\n");
            return;
        }
        char enumElementBuf[MAXENUMELEMENTLEN];
        for (int i = 0 ; i < numOfEnumElements ; i++) {
            strncpy((char*)(enumeration[i]), extractEnumElement(enums, i, enumElementBuf), MAXENUMELEMENTLEN);
            enumeration[i][MAXENUMELEMENTLEN-1] = '\0';
        }
        fwrite(enumeration, sizeof(enum_t)*numOfEnumElements, 1, treeFp);
        free(enumeration);
    }
    int functionLen = (int)strlen(function);
    fwrite(&functionLen, sizeof(int), 1, treeFp);
    if (functionLen > 0)
        fwrite(function, sizeof(char)*functionLen, 1, treeFp);
}

void createNativeCnode(char*fname, char* name, char* type, char* uuid, char* descr, int children, char* datatype, char* min, char* max, char* unit, char* enums, char* function) {
    treeFp = fopen(fname, "a");
    if (treeFp == NULL) {
        printf("Could not open file for writing of tree.\n");
        return;
    }
    writeNodeData(name, type, uuid, descr, children, datatype, min, max, unit, enums, function);
    fclose(treeFp);
}

void writeProperty(char* propName, char* propDescr, char* propType, char* propFormat, char* propUnit, char* propValue) {
printf("Rbranch:propName=%s, propDescr=%s, propType=%s, propFormat=%s, propUnit=%s, propValue=%s\n", propName, propDescr, propType, propFormat, propUnit, propValue);
    propertyDefinition_t propertyDefinition;

    strcpy(propertyDefinition.propName, propName);
    strcpy(propertyDefinition.propDescr, propDescr);
    propertyDefinition.propType = stringToTypeDef(propType);
    strcpy(propertyDefinition.propFormat, propFormat);
    strcpy(propertyDefinition.propUnit, propUnit);
    strcpy(propertyDefinition.propValue, propValue);
    fwrite(&propertyDefinition, sizeof(propertyDefinition_t), 1, treeFp);
}

void writeRbranchNodeData(char* name, char* type, char* uuid, char* descr, int children, char* childType, int numOfProperties, char** propNames, char** propDescrs, char** propTypes, char** propFormats, char** propUnits, char** propValues) {
printf("Rbranch:Name=%s, Type=%s, Descr=%s, children=%d, childType=%s, numOfProps=%d\n", name, type, descr, children, childType, numOfProperties);
    writeCommonPart(name, type, uuid, descr, children);
    int childTypeLen = strlen(childType);
    fwrite(&childTypeLen, sizeof(int), 1, treeFp);
    fwrite(&numOfProperties, sizeof(int), 1, treeFp);
    for (int i = 0 ; i < numOfProperties ; i++) {
        writeProperty(propNames[i], propDescrs[i], propTypes[i], propFormats[i], propUnits[i], propValues[i]);
    }
}

int propertyPosition(objectTypes_t objectType, char* memberName) {
        if (strcmp(memberName, "id" ) == 0)
            return 0;
        if (strcmp(memberName, "name" ) == 0)
            return 1;
        if (strcmp(memberName, "uri" ) == 0)
            return 2;
        if (objectType == MEDIACOLLECTION && strcmp(memberName, "items" ) == 0)
            return 3;
        return -1;
}

int parseRefString(char* refString, elementRef_t** elementRef) {
    int numOfRefs = 0;
    char* refStart = strchr(refString, '\'');
    char* refEnd;

    while (refStart != NULL) {
        numOfRefs++;
        refEnd = strchr(refStart+1, '\'');
        refStart = strchr(refEnd+1, '\'');
    }
printf("parseRefString: %d refs\n", numOfRefs);
    *elementRef = (elementRef_t*) malloc(sizeof(elementRef_t)*numOfRefs);
    refStart = strchr(refString, '\'')+1;
    char* thisRef;
    for (int i = 0 ; i < numOfRefs ; i++) {
        refEnd = strchr(refStart, '\'');
        thisRef = (*elementRef)[i];
        strncpy(thisRef, refStart, refEnd-refStart);
        thisRef[refEnd-refStart] = '\0';
printf("Ref[%d]=%s\n",i, thisRef);
        refStart = strchr(refEnd+1, '\'')+1;
    }
    return numOfRefs;
}

void populateAndWriteObject(int objectType, int numOfElems, char** memberName, char** memberValue) {
    switch (objectType) {
        case MEDIACOLLECTION:
        {
            mediaCollectionObject_t mediaCollectionObject;
            mediaCollectionObject.objectType = MEDIACOLLECTION;
            for (int i = 0 ; i < numOfElems ; i++) {
                int pos;
                switch (pos = propertyPosition(objectType, memberName[i])) {
                    case 0:
                        strcpy(mediaCollectionObject.id, memberValue[i]);
                    break;
                    case 1:
                        strcpy(mediaCollectionObject.name, memberValue[i]);
                    break;
                    case 2:
                        strcpy(mediaCollectionObject.uri, memberValue[i]);
                    break;
                    case 3:
                    {
                        elementRef_t* itemRef;
                        int numOfRefs = parseRefString(memberValue[i], &itemRef);
                        mediaCollectionObject.numOfItems = numOfRefs;
                        mediaCollectionObject.items = itemRef;
                    }
                    break;
                    default:
                        printf("Unknown member of mediaCollectionObject=%s!!\n", memberName[i]);
                    break;
                } //switch
            } // for
            fwrite(&mediaCollectionObject, sizeof(mediaCollectionObject_t), 1, treeFp);
            fwrite(mediaCollectionObject.items, sizeof(elementRef_t)*mediaCollectionObject.numOfItems, 1, treeFp);
        }
        break;
        case MEDIAITEM:
        {
            mediaItemObject_t mediaItemObject;
            mediaItemObject.objectType = MEDIAITEM;
            for (int i = 0 ; i < numOfElems ; i++) {
                int pos;
                switch (pos = propertyPosition(objectType, memberName[i])) {
                    case 0:
                        strcpy(mediaItemObject.id, memberValue[i]);
                    break;
                    case 1:
                        strcpy(mediaItemObject.name, memberValue[i]);
                    break;
                    case 2:
                        strcpy(mediaItemObject.uri, memberValue[i]);
                    break;
                    default:
                        printf("Unknown member of mediaItemObject=%s!!\n", memberName[i]);
                    break;
                } //switch
            } // for
            fwrite(&mediaItemObject, sizeof(mediaItemObject_t), 1, treeFp);
        }
        break;
    }// switch
}

void writeElementNodeData(char* name, char* type, char* uuid, char* descr, int children, int numOfElems, char** memberName, char** memberValue) {
printf("Element:Name=%s, Type=%s, Descr=%s, children=%d, numOfElems=%d\n", name, type, descr, children, numOfElems);
    writeCommonPart(name, type, uuid, descr, children);
    for (int i = 0 ; i < numOfElems ; i++) {
        printf("%s:%s\n", memberName[i], memberValue[i]);
    }
    populateAndWriteObject(objectType, numOfElems, memberName, memberValue);
}

int convertObjectType(char* childType) {
    if (strcmp(childType, "mediaCollectionObject") == 0)
        return MEDIACOLLECTION;
    if (strcmp(childType, "mediaItemObject") == 0)
        return MEDIAITEM;
    return -1;
}

void createNativeCnodeRbranch(char*fname, char* name, char* type, char* uuid, char* descr, int children, char* childType, int numOfProperties, char** propNames, char** propDescrs, char** propTypes, char** propFormats, char** propUnits, char** propValues) {
    treeFp = fopen(fname, "a");
    if (treeFp == NULL) {
        printf("Could not open file for writing of tree.\n");
        return;
    }
    writeRbranchNodeData(name, type, uuid, descr, children, childType, numOfProperties, propNames, propDescrs, propTypes, propFormats, propUnits, propValues);
    objectType = convertObjectType(childType);
    fclose(treeFp);
}

void createNativeCnodeElement(char*fname, char* name, char* type, char* uuid, char* descr, int children, int numOfElems, char** memberName, char** memberValue) {
    treeFp = fopen(fname, "a");
    if (treeFp == NULL) {
        printf("Could not open file for writing of tree.\n");
        return;
    }
    writeElementNodeData(name, type, uuid, descr, children, numOfElems, memberName, memberValue);
    fclose(treeFp);
}
