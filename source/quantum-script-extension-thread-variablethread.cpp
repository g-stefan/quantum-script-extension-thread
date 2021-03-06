﻿//
// Quantum Script Extension Thread
//
// Copyright (c) 2020-2021 Grigore Stefan <g_stefan@yahoo.com>
// Created by Grigore Stefan <g_stefan@yahoo.com>
//
// MIT License (MIT) <http://opensource.org/licenses/MIT>
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "quantum-script-context.hpp"
#include "quantum-script-variablenumber.hpp"
#include "quantum-script-variablestring.hpp"
#include "quantum-script-variableobject.hpp"
#include "quantum-script-variableresource.hpp"

#include "quantum-script-extension-thread.hpp"
#include "quantum-script-extension-thread-variablethread.hpp"

namespace Quantum {
	namespace Script {
		namespace Extension {
			namespace Thread {

				using namespace XYO;

				const char *VariableThread::typeThreadKey = "{EC4DDB48-CB72-4668-9F29-ACED0BD31287}";
				const void *VariableThread::typeThread;
				const char *VariableThread::strTypeThread = "Thread.Thread";

				String VariableThread::getType() {
					return strTypeThread;
				};

				Variable *VariableThread::newVariable() {
					return (Variable *) TMemory<VariableThread>::newMemory();
				};

				Variable &VariableThread::operatorReference(Symbol symbolId) {
					return operatorReferenceX(symbolId, (Extension::Thread::getContext())->prototypeThread->prototype);
				};

				Variable *VariableThread::instancePrototype() {
					return (Extension::Thread::getContext())->prototypeThread->prototype;
				};

				void VariableThread::activeDestructor() {
					join();
					requestToTerminateThread = false;
					returnValue = nullptr;
					executive = nullptr;
					sourceCode = "";
					startedOk = false;
					threadEnded = true;
					returnedValue = Variable::newVariable();
				};

				Variable *VariableThread::clone(SymbolList &inSymbolList) {
					VariableThread *out = (VariableThread *)newVariable();
					return out;
				};

				bool VariableThread::toBoolean() {
					return true;
				};

				String VariableThread::toString() {
					return strTypeThread;
				};

				// ---

				static void threadProcedure_(VariableThread *thread) {
					TPointer<Variable> threadFunction;
					TPointer<Variable> functionThis;
					TPointer<Variable> functionArguments;
					if(ExecutiveX::initExecutive(thread->executive->mainCmdN, thread->executive->mainCmdS, thread->executive->applicationInitExecutive)) {
						if(ExecutiveX::executeString((char *)thread->sourceCode.value())) {
							threadFunction = ExecutiveX::returnValue();

							// CurrentThread.this_=thread;
							Symbol currentThreadSymbol = Context::getSymbol("CurrentThread");
							if(VariableUndefined::isVariableUndefined((Context::getGlobalObject())->operatorReferenceOwnProperty(currentThreadSymbol))) {
								((Context::getGlobalObject())->operatorReferenceOwnProperty(currentThreadSymbol))=VariableObject::newVariable();
							};

							TPointerX<Variable> &currenThread = (Context::getGlobalObject())->operatorReferenceOwnProperty(currentThreadSymbol);
							(currenThread->operatorReferenceOwnProperty(Context::getSymbol("this_")))=VariableResource::newVariable(thread, nullptr);

							try {
								functionThis = thread->functionThis->clone(*(thread->symbolList));
							} catch(...) {
								functionThis = Context::getValueUndefined();
							};

							try {
								if(VariableArray::isVariableArray(thread->functionArguments)) {
									functionArguments = thread->functionArguments->clone(*(thread->symbolList));
									if(!VariableArray::isVariableArray(functionArguments)) {
										functionArguments = VariableArray::newVariable();
									};
								} else {
									functionArguments = VariableArray::newVariable();
								};
							} catch(...) {
								functionArguments = VariableArray::newVariable();
							};

							thread->startedOk = true;
							thread->threadStarted.notify();

							try {

								thread->returnValue = threadFunction->functionApply(functionThis, (VariableArray *)functionArguments.value());
								if(!thread->returnValue) {
									thread->returnValue = Context::getValueUndefined();
								};

							} catch(...) {
								thread->returnValue = Context::getValueUndefined();
							};

							thread->functionSymbolList = &Context::getSymbolList();
							functionArguments.deleteMemory();
							thread->threadTerminated.notify();
							thread->threadWaitCopyReturnValue.wait();
							thread->returnValue.deleteMemory();

							ExecutiveX::endProcessing();
							return;
						};
					};
					thread->threadStarted.notify();
					ExecutiveX::endProcessing();
				};

				bool VariableThread::start(Executive *executive_, String sourceCode_, Variable *functionThis_, Variable *functionArguments_) {
					join();
					executive = executive_;
					sourceCode = sourceCode_;
					functionThis = functionThis_;
					functionArguments = functionArguments_;
					startedOk = false;
					threadEnded = false;
					requestToTerminateThread = false;
					symbolList = &Context::getSymbolList();
					if(thread.start((ThreadProcedure)threadProcedure_, this)) {
						threadStarted.wait();
						if(startedOk) {
							return true;
						};
						thread.join();
					};
					return false;
				};

				void VariableThread::join() {
					if(thread.isRunning()) {
						if(!threadEnded) {
							threadEnded = true;
							threadTerminated.wait();
							returnedValue = returnValue->clone(*functionSymbolList);
							threadWaitCopyReturnValue.notify();
						};
						thread.join();
					};
				};

				bool VariableThread::isRunning() {
					if(thread.isRunning()) {
						if(!threadEnded) {
							if(threadTerminated.peek()) {
								threadEnded = true;
								threadTerminated.wait();
								returnedValue = returnValue->clone(*functionSymbolList);
								threadWaitCopyReturnValue.notify();
								return false;
							};
						};
						return true;
					};
					return false;
				};

			};
		};
	};
};


