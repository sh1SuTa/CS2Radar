#pragma warning(disable: 4100 4047 4024)
#pragma once
#include<ntifs.h>

extern "C" {
	NTKERNELAPI NTSTATUS IoCreateDriver(PUNICODE_STRING Driver, PDRIVER_INITIALIZE function);
	//Ŀ����̣�Ŀ���ַ��Դ���̣�Դ��ַ����С��ģʽ�����ش�С
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
		DbgPrint("[+] �����豸called.\n");
		UNREFERENCED_PARAMETER(device_object);
		IoCompleteRequest(irp,IO_NO_INCREMENT);
		return irp->IoStatus.Status;
	}
	NTSTATUS close(PDEVICE_OBJECT device_object, PIRP irp) {
		DbgPrint("[+] �ر��豸CloseHandle.\n");
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
			DbgPrint("[-] stack_irp��request���п�ֵ\n");
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
				DbgPrint("[+]Ŀ��ĵ�ַΪ%p.\n", request->target);
				status = MmCopyVirtualMemory(target_process, request->target, PsGetCurrentProcess(), request->buffer, request->size, 
					KernelMode, &request->return_size);
				DbgPrint("[+]����ֵ����ĵ�ַΪ%p.\n", request->buffer);
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
	DbgPrint("[+]����ж������\n");



	// ɾ����������
	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\SexyDriver");
	NTSTATUS status = IoDeleteSymbolicLink(&symLink);
	if (status != STATUS_SUCCESS) {
		DbgPrint("[-]ɾ��ʧ�� symbolic link: %08x\n", status);
	}
	else {
		DbgPrint("[+]�ɹ�ɾ�� symbolic link\n");
	}

	// ɾ���豸���� (IoDeleteDevice ���� void�����ܸ�ֵ�� status)

	//IoDeleteDevice(pDeviceObject);  // ֱ�ӵ��ã�����Ҫ���淵��ֵ
	IoDeleteDevice(pDriverObject->DeviceObject);
	DbgPrint("[+]IoDeleteDevice ɾ����pDriverObject->DeviceObject\n");


	return ;
}
NTSTATUS driver_main(PDRIVER_OBJECT driver_object, PUNICODE_STRING register_path) {
	DbgPrint("[+] ��ʼ����driver_main\n");
	UNREFERENCED_PARAMETER(register_path);
	UNICODE_STRING device_name = {};
	RtlInitUnicodeString(&device_name, L"\\Device\\SexyDriver");
	

	PDEVICE_OBJECT device_object = nullptr;
	
	NTSTATUS status = IoCreateDevice(driver_object, 0, &device_name, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &device_object);
	if (status == STATUS_OBJECT_NAME_COLLISION) {
		debug_print("[-] �豸���Ƴ�ͻ������ʹ��Ψһ����\n");
		return status;
	}
	else if (status != STATUS_SUCCESS) {
		debug_print("[-] ���������豸ʧ��\n");
		return status;
	}
	
	UNICODE_STRING symbolic_link = {};
	RtlInitUnicodeString(&symbolic_link, L"\\??\\SexyDriver");
	status = IoCreateSymbolicLink(&symbolic_link, &device_name);
	if (status!=STATUS_SUCCESS)
	{
		debug_print("[-] ������������ʧ��\n");
		IoDeleteDevice(device_object);
		return status;
	}
	
	
	SetFlag(device_object->Flags, DO_BUFFERED_IO);//������um/km
	driver_object->MajorFunction[IRP_MJ_CREATE] = driver::create;
	driver_object->MajorFunction[IRP_MJ_CLOSE] = driver::close;
	driver_object->MajorFunction[IRP_MJ_DEVICE_CONTROL] = driver::device_control;
	driver_object->DriverUnload = UnloadDriver;
	/* 
	DO_DEVICE_INITIALIZING
	��־λ����ָʾ�豸�������ڳ�ʼ�������豸�����ʼ����ɺ���������־λ��ʾ�豸�����Ѿ�׼���ã����Խ��� I/O ���� */
	ClearFlag(device_object->Flags, DO_DEVICE_INITIALIZING);
	debug_print("[+] ��ʼ�豸���\n"); 
	return status;
}
//kdmapper������Ҫ��PDRIVER_OBJECT
 NTSTATUS DriverEntry() {
	
	
	UNICODE_STRING driver_name = {  };
	RtlInitUnicodeString(&driver_name, L"\\Driver\\SexyDriver");
	NTSTATUS status = IoCreateDriver(&driver_name, &driver_main);
	DbgPrint("[+] IoCreateDriver called��%x\n",status);
	
	return status;
}

