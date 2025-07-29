//===== Copyright © 1996-2006, Valve Corporation, All rights reserved. ======//
//
// Purpose: 	ExprSimplifier builds a binary tree from an infix expression (in the
//				form of a character array).
//
//===========================================================================//

#ifndef EXPREVALUATOR_H
#define EXPREVALUATOR_H

#if defined( _WIN32 )
#pragma once
#endif

static const char OR_OP   = '|';
static const char AND_OP  = '&';
static const char NOT_OP  = '!';

#define MAX_IDENTIFIER_LEN	128
enum Kind {CONDITIONAL, NOT, LITERAL};

struct ExprNode
{
	ExprNode *left;  // left sub-expression
	ExprNode *right; // right sub-expression
	Kind kind;       // kind of node this is
	union 
	{
		char cond;	// the conditional
		bool value;	// the value
	} data;
};

typedef ExprNode *ExprTree;

// callback to evaluate a $<symbol> during evaluation, return true or false
typedef bool (*GetSymbolProc_t)( const char *pKey, void *pData );
typedef void (*SyntaxErrorProc_t)( const char *pReason, void *pData );

class CExpressionEvaluator
{
public:
	DLL_CLASS_IMPORT CExpressionEvaluator();
	DLL_CLASS_IMPORT ~CExpressionEvaluator();

	DLL_CLASS_IMPORT bool Evaluate( 
		bool &result, const char *pInfixExpression,
		GetSymbolProc_t pGetSymbolProc = nullptr, SyntaxErrorProc_t pSyntaxErrorProc = nullptr,
		void *pSymbolProcAdditionalData = nullptr, void *pErrorProcAdditionalData = nullptr
	);

	DLL_CLASS_IMPORT bool EvaluateAsUnmarkedSymbols(
		bool &result, const char *pInfixExpression,
		GetSymbolProc_t pGetSymbolProc = nullptr, SyntaxErrorProc_t pSyntaxErrorProc = nullptr,
		void *pSymbolProcAdditionalData = nullptr, void *pErrorProcAdditionalData = nullptr
	);

private:
	CExpressionEvaluator( CExpressionEvaluator & ) = delete;

	ExprTree			m_ExprTree;							// Tree representation of the expression
	char				m_CurToken;							// Current token read from the input expression
	const char			*m_pExpression;						// Array of the expression characters
	int					m_CurPosition;						// Current position in the input expression
	char				m_Identifier[MAX_IDENTIFIER_LEN];	// Stores the identifier string
	GetSymbolProc_t		m_pGetSymbolProc;
	SyntaxErrorProc_t	m_pSyntaxErrorProc;
	void				*m_pSymbolProcData;
	void				*m_pErrorProcData;
	bool				m_bSetup;
};

#endif
