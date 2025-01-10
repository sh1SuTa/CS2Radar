#pragma warning(disable: 4100 4047 4024)
#pragma once
#include<ntifs.h>

extern "C" {
	NTKERNELAPI NTSTATUS IoCreateDriver(PUNICODE_STRING Driver, PDRIVER_INITIALIZE function);
	//目标进程，目标地址，源进程，源地址，大小，模式，返回大小
	NTKERNELAPI NTSTATUS MmCopyVirtualMemory(PEPROCESS process, PVOID address,
		PEPROCESS targetProcess, PVOID targetAddress, SIZE_T bufferSize,KPROCESSOR_MODE mode,PSIZE_T returnSize);
}

void debug_print(PCSTR text) {
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, text));

}
namespace driver {
	namespace codes {
		const ULONG attach=CTL_CODE(FILE_DEVICE_UNKNOWN, 0X696, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
		const ULONG read=CTL_CODE(FILE_DEVICE_UNKNOWN, 0X697, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
		const ULONG write=CTL_CODE(FILE_DEVICE_UNKNOWN, 0X698, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
	}
	struct Request {
		HANDLE process_id;
		PVOID target;
		PVOID buffer;
		SIZE_T size;
		SIZE_T return_size;
	};
	NTSTATUS create(PDEVICE_OBJECT device_object, PIRP irp){
		DbgPrint("[+] 创建设备called.\n");
		UNREFERENCED_PARAMETER(device_object);
		IoCompleteRequest(irp,IO_NO_INCREMENT);
		return irp->IoStatus.Status;
	}
	NTSTATUS close(PDEVICE_OBJECT device_object, PIRP irp) {
		DbgPrint("[+] 关闭设备CloseHandle.\n");
		UNREFERENCED_PARAMETER(device_object);
		IoCompleteRequest(irp, IO_NO_INCREMENT);
		return irp->IoStatus.Status;
	}
	NTSTATUS device_control(PDEVICE_OBJECT device_object, PIRP irp) {
		UNREFERENCED_PARAMETER(device_object);
		
		NTSTATUS status = STATUS_UNSUCCESSFUL;
		PIO_STACK_LOCATION stack_irp = IoGetCurrentIrpStackLocation(irp);
		
		auto request = reinterpret_cast<Request*>(irp->AssociatedIrp.SystemBuffer);
		if (stack_irp==nullptr||request==nullptr)
		{
			IoCompleteRequest(irp, IO_NO_INCREMENT); 
			DbgPrint("[-] stack_irp和request中有空值\n");
			return status;
		}
		static PEPROCESS target_process = nullptr;
		const ULONG control_code = stack_irp->Parameters.DeviceIoControl.IoControlCode;
		
		switch (control_code)
		{
		case codes::attach:
			DbgPrint("[+] attach called.\n");
			status = PsLookupProcessByProcessId(request->process_id, &target_process);
			break;
		case codes::read:
			if (target_process!=nullptr)
			{
				DbgPrint("[+]目标的地址为%p.\n", request->target);
				status = MmCopyVirtualMemory(target_process, request->target, PsGetCurrentProcess(), request->buffer, request->size, 
					KernelMode, &request->return_size);
				DbgPrint("[+]读到值存入的地址为%p.\n", request->buffer);
			}
			break;
		case codes::write:
			if (target_process != nullptr)
			{
				status = MmCopyVirtualMemory(PsGetCurrentProcess(), request->buffer,target_process, request->target, request->size, 
					KernelMode, &request->return_size);
			}
			break;
		default:
			break;
		}
		irp->IoStatus.Status = status;
		irp->IoStatus.Information = sizeof(Request);


		IoCompleteRequest(irp, IO_NO_INCREMENT);
		return irp->IoStatus.Status;
	}
}

void UnloadDriver(PDRIVER_OBJECT pDriverObject)
{
	DbgPrint("[+]正在卸载驱动\n");



	// 删除符号链接
	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\SexyDriver");
	NTSTATUS status = IoDeleteSymbolicLink(&symLink);
	if (status != STATUS_SUCCESS) {
		DbgPrint("[-]删除失败 symbolic link: %08x\n", status);
	}
	else {
		DbgPrint("[+]成功删除 symbolic link\n");
	}

	// 删除设备对象 (IoDeleteDevice 返回 void，不能赋值给 status)

	//IoDeleteDevice(pDeviceObject);  // 直接调用，不需要保存返回值
	IoDeleteDevice(pDriverObject->DeviceObject);
	DbgPrint("[+]IoDeleteDevice 删除了pDriverObject->DeviceObject\n");


	return ;
}
NTSTATUS driver_main(PDRIVER_OBJECT driver_object, PUNICODE_STRING register_path) {
	DbgPrint("[+] 开始进入driver_main\n");
	UNREFERENCED_PARAMETER(register_path);
	UNICODE_STRING device_name = {};
	RtlInitUnicodeString(&device_name, L"\\Device\\SexyDriver");
	

	PDEVICE_OBJECT device_object = nullptr;
	
	NTSTATUS status = IoCreateDevice(driver_object, 0, &device_name, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &device_object);
	if (status == STATUS_OBJECT_NAME_COLLISION) {
		debug_print("[-] 设备名称冲突，尝试使用唯一名称\n");
		return status;
	}
	else if (status != STATUS_SUCCESS) {
		debug_print("[-] 创建驱动设备失败\n");
		return status;
	}
	
	UNICODE_STRING symbolic_link = {};
	RtlInitUnicodeString(&symbolic_link, L"\\??\\SexyDriver");
	status = IoCreateSymbolicLink(&symbolic_link, &device_name);
	if (status!=STATUS_SUCCESS)
	{
		debug_print("[-] 建立符号链接失败\n");
		IoDeleteDevice(device_object);
		return status;
	}
	
	
	SetFlag(device_object->Flags, DO_BUFFERED_IO);//允许发送um/km
	driver_object->MajorFunction[IRP_MJ_CREATE] = driver::create;
	driver_object->MajorFunction[IRP_MJ_CLOSE] = driver::close;
	driver_object->MajorFunction[IRP_MJ_DEVICE_CONTROL] = driver::device_control;
	driver_object->DriverUnload = UnloadDriver;
	/* 
	DO_DEVICE_INITIALIZING
	标志位用于指示设备对象正在初始化。当设备对象初始化完成后，清除这个标志位表示设备对象已经准备好，可以接收 I/O 请求 */
	ClearFlag(device_object->Flags, DO_DEVICE_INITIALIZING);
	debug_print("[+] 初始设备完成\n"); 
	return status;
}
//kdmapper加载需要无PDRIVER_OBJECT
 NTSTATUS DriverEntry() {
	
	
	UNICODE_STRING driver_name = {  };
	RtlInitUnicodeString(&driver_name, L"\\Driver\\SexyDriver");
	NTSTATUS status = IoCreateDriver(&driver_name, &driver_main);
	DbgPrint("[+] IoCreateDriver called：%x\n",status);
	
	return status;
}

